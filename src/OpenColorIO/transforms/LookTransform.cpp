// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include <algorithm>
#include <iterator>

#include "LookParse.h"
#include "ops/noop/NoOps.h"
#include "OpBuilders.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"


namespace OCIO_NAMESPACE
{
LookTransformRcPtr LookTransform::Create()
{
    return LookTransformRcPtr(new LookTransform(), &deleter);
}

void LookTransform::deleter(LookTransform* t)
{
    delete t;
}

class LookTransform::Impl
{
public:
    TransformDirection m_dir;
    std::string m_src;
    std::string m_dst;
    std::string m_looks;

    Impl() :
        m_dir(TRANSFORM_DIR_FORWARD)
    { }

    Impl(const Impl &) = delete;

    ~Impl()
    { }

    Impl& operator= (const Impl & rhs)
    {
        if (this != &rhs)
        {
            m_dir = rhs.m_dir;
            m_src = rhs.m_src;
            m_dst = rhs.m_dst;
            m_looks = rhs.m_looks;
        }
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////

LookTransform::LookTransform()
    : m_impl(new LookTransform::Impl)
{
}

TransformRcPtr LookTransform::createEditableCopy() const
{
    LookTransformRcPtr transform = LookTransform::Create();
    *(transform->m_impl) = *m_impl;
    return transform;
}

LookTransform::~LookTransform()
{
    delete m_impl;
    m_impl = NULL;
}

LookTransform& LookTransform::operator= (const LookTransform & rhs)
{
    if (this != &rhs)
    {
        *m_impl = *rhs.m_impl;
    }
    return *this;
}

TransformDirection LookTransform::getDirection() const noexcept
{
    return getImpl()->m_dir;
}

void LookTransform::setDirection(TransformDirection dir) noexcept
{
    getImpl()->m_dir = dir;
}

void LookTransform::validate() const
{
    Transform::validate();

    if (getImpl()->m_src.empty())
    {
        throw Exception("LookTransform: empty source color space name");
    }

    if (getImpl()->m_dst.empty())
    {
        throw Exception("LookTransform: empty destination color space name");
    }
}

const char * LookTransform::getSrc() const
{
    return getImpl()->m_src.c_str();
}

void LookTransform::setSrc(const char * src)
{
    getImpl()->m_src = src;
}

const char * LookTransform::getDst() const
{
    return getImpl()->m_dst.c_str();
}

void LookTransform::setDst(const char * dst)
{
    getImpl()->m_dst = dst;
}

void LookTransform::setLooks(const char * looks)
{
    getImpl()->m_looks = looks;
}

const char * LookTransform::getLooks() const
{
    return getImpl()->m_looks.c_str();
}

std::ostream& operator<< (std::ostream& os, const LookTransform& t)
{
    os << "<LookTransform";
    os << " src=" << t.getSrc();
    os << ", dst=" << t.getDst();
    os << ", looks=" << t.getLooks();
    os << ", direction=" << TransformDirectionToString(t.getDirection());
    os << ">";
    return os;
}

////////////////////////////////////////////////////////////////////////////

namespace
{

void RunLookTokens(OpRcPtrVec & ops,
                    ConstColorSpaceRcPtr & currentColorSpace,
                    bool skipColorSpaceConversions,
                    const Config& config,
                    const ConstContextRcPtr & context,
                    const LookParseResult::Tokens & lookTokens)
{
    if(lookTokens.empty()) return;

    for(unsigned int i=0; i<lookTokens.size(); ++i)
    {
        const std::string & lookName = lookTokens[i].name;

        if(lookName.empty()) continue;

        ConstLookRcPtr look = config.getLook(lookName.c_str());
        if(!look)
        {
            std::ostringstream os;
            os << "RunLookTokens error. ";
            os << "The specified look, '" << lookName;
            os << "', cannot be found. ";
            if(config.getNumLooks() == 0)
            {
                os << " (No looks defined in config)";
            }
            else
            {
                os << " (looks: ";
                for(int ii=0; ii<config.getNumLooks(); ++ii)
                {
                    if(ii != 0) os << ", ";
                    os << config.getLookNameByIndex(ii);
                }
                os << ")";
            }

            throw Exception(os.str().c_str());
        }

        // Put the new ops into a temp array, to see if it's a no-op
        // If it is a no-op, dont bother doing the colorspace conversion.
        OpRcPtrVec tmpOps;

        if(lookTokens[i].dir == TRANSFORM_DIR_FORWARD)
        {
            CreateLookNoOp(tmpOps, lookName);
            if(look->getTransform())
            {
                BuildOps(tmpOps, config, context, look->getTransform(), TRANSFORM_DIR_FORWARD);
            }
            else if(look->getInverseTransform())
            {
                BuildOps(tmpOps, config, context, look->getInverseTransform(), TRANSFORM_DIR_INVERSE);
            }
        }
        else if(lookTokens[i].dir == TRANSFORM_DIR_INVERSE)
        {
            CreateLookNoOp(tmpOps, std::string("-") + lookName);
            if(look->getInverseTransform())
            {
                BuildOps(tmpOps, config, context, look->getInverseTransform(), TRANSFORM_DIR_FORWARD);
            }
            else if(look->getTransform())
            {
                BuildOps(tmpOps, config, context, look->getTransform(), TRANSFORM_DIR_INVERSE);
            }
        }
        else
        {
            std::ostringstream os;
            os << "BuildLookOps error. ";
            os << "The specified look, '" << lookTokens[i].name;
            os << "' has an ill-defined transform direction.";
            throw Exception(os.str().c_str());
        }

        if (!tmpOps.isNoOp())
        {
            if(!skipColorSpaceConversions)
            {
                ConstColorSpaceRcPtr processColorSpace = config.getColorSpace(look->getProcessSpace());
                if(!processColorSpace)
                {
                    std::ostringstream os;
                    os << "RunLookTokens error. ";
                    os << "The specified look, '" << lookTokens[i].name;
                    os << "', requires processing in the ColorSpace, '";
                    os << look->getProcessSpace() << "' which is not defined.";
                    throw Exception(os.str().c_str());
                }

                BuildColorSpaceOps(ops, config, context,
                                    currentColorSpace,
                                    processColorSpace);
                currentColorSpace = processColorSpace;
            }

            ops += tmpOps;
        }
    }
}

} // anon namespace

////////////////////////////////////////////////////////////////////////////

void BuildLookOps(OpRcPtrVec & ops,
                    const Config& config,
                    const ConstContextRcPtr & context,
                    const LookTransform & lookTransform,
                    TransformDirection dir)
{
    ConstColorSpaceRcPtr src, dst;
    src = config.getColorSpace( lookTransform.getSrc() );
    dst = config.getColorSpace( lookTransform.getDst() );

    if(!src)
    {
        std::ostringstream os;
        os << "BuildLookOps error.";
        os << "The specified lookTransform specifies a src colorspace, '";
        os <<  lookTransform.getSrc() << "', which is not defined.";
        throw Exception(os.str().c_str());
    }

    if(!dst)
    {
        std::ostringstream os;
        os << "BuildLookOps error.";
        os << "The specified lookTransform specifies a dst colorspace, '";
        os <<  lookTransform.getDst() << "', which is not defined.";
        throw Exception(os.str().c_str());
    }

    LookParseResult looks;
    looks.parse(lookTransform.getLooks());

    // We must handle the inverse src/dst colorspace transformation explicitly.
    if(dir == TRANSFORM_DIR_INVERSE)
    {
        std::swap(src, dst);
        looks.reverse();
    }
    else if(dir == TRANSFORM_DIR_UNKNOWN)
    {
        std::ostringstream os;
        os << "BuildLookOps error. A valid transform direction must be specified.";
        throw Exception(os.str().c_str());
    }

    ConstColorSpaceRcPtr currentColorSpace = src;
    BuildLookOps(ops,
                    currentColorSpace,
                    false,
                    config,
                    context,
                    looks);

    BuildColorSpaceOps(ops, config, context,
                        currentColorSpace,
                        dst);
}

void BuildLookOps(OpRcPtrVec & ops,
                    ConstColorSpaceRcPtr & currentColorSpace,
                    bool skipColorSpaceConversions,
                    const Config& config,
                    const ConstContextRcPtr & context,
                    const LookParseResult & looks)
{
    const LookParseResult::Options & options = looks.getOptions();

    if(options.empty())
    {
        // Do nothing
    }
    else if(options.size() == 1)
    {
        // As an optimization, if we only have a single look option,
        // just push back onto the final location
        RunLookTokens(ops,
                        currentColorSpace,
                        skipColorSpaceConversions,
                        config,
                        context,
                        options[0]);
    }
    else
    {
        // If we have multiple look options, try each one in order,
        // and if we can create the ops without a missing file exception,
        // push back it's results and return

        bool success = false;
        std::ostringstream os;

        OpRcPtrVec tmpOps;
        ConstColorSpaceRcPtr cs;

        for(unsigned int i=0; i<options.size(); ++i)
        {
            cs = currentColorSpace;
            tmpOps.clear();

            try
            {
                RunLookTokens(tmpOps,
                                cs,
                                skipColorSpaceConversions,
                                config,
                                context,
                                options[i]);
                success = true;
                break;
            }
            catch(ExceptionMissingFile & e)
            {
                if(i != 0) os << "  ...  ";

                os << "(";
                LookParseResult::serialize(os, options[i]);
                os << ") " << e.what();
            }
        }

        if(success)
        {
            currentColorSpace = cs;
            ops += tmpOps;
        }
        else
        {
            throw ExceptionMissingFile(os.str().c_str());
        }
    }
}
} // namespace OCIO_NAMESPACE
