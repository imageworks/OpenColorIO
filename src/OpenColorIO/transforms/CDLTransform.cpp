// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <fstream>
#include <sstream>
#include <string.h>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/cdl/CDLParser.h"
#include "CDLTransform.h"
#include "MathUtils.h"
#include "Mutex.h"
#include "OpBuilders.h"
#include "ops/CDL/CDLOps.h"
#include "ops/Exponent/ExponentOps.h"
#include "ops/Matrix/MatrixOps.h"
#include "ParseUtils.h"
#include "pystring/pystring.h"


OCIO_NAMESPACE_ENTER
{
    namespace
    {

 /*
<ColorCorrection id="shot 042">
    <SOPNode>
        <Description>Cool look for forest scenes</Description>
        <Slope>1 1 1</Slope>
        <Offset>0 0 0</Offset>
        <Power>1 1 1</Power>
    </SOPNode>
    <SatNode>
        <Saturation>1</Saturation>
    </SatNode>
</ColorCorrection>
        
 */
        
        std::string BuildXML(const CDLTransform & cdl)
        {
            std::ostringstream os;
            std::string id(ConvertSpecialCharToXmlToken(cdl.getID()));
            os << "<ColorCorrection id=\"" << id << "\">\n";
            os << "    <SOPNode>\n";
            std::string desc(ConvertSpecialCharToXmlToken(cdl.getDescription()));
            os << "        <Description>" << desc << "</Description>\n";
            float slopeval[3];
            cdl.getSlope(slopeval);
            os << "        <Slope>" << FloatVecToString(slopeval, 3) << "</Slope>\n";
            float offsetval[3];
            cdl.getOffset(offsetval);
            os << "        <Offset>" << FloatVecToString(offsetval, 3)  << "</Offset>\n";
            float powerval[3];
            cdl.getPower(powerval);
            os << "        <Power>" << FloatVecToString(powerval, 3)  << "</Power>\n";
            os << "    </SOPNode>\n";
            os << "    <SatNode>\n";
            os << "        <Saturation>" << FloatToString(cdl.getSat()) << "</Saturation>\n";
            os << "    </SatNode>\n";
            os << "</ColorCorrection>";

            return os.str();
        }
    }
    
    void LoadCDL(CDLTransform & cdl, const char * xml)
    {
        if(!xml || (strlen(xml) == 0))
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << "Null string provided.";
            throw Exception(os.str().c_str());
        }
        
        std::string xmlstring(xml);
        std::istringstream istream(xmlstring);

        CDLParser parser("xml string");
        parser.parse(istream);

        if (!parser.isCC())
        {
            throw Exception("Error loading CDL xml. ColorCorrection expected.");
        }

        CDLTransformRcPtr cdlRcPtr;
        parser.getCDLTransform(cdlRcPtr);

        cdl.setID(cdlRcPtr->getID());
        cdl.setDescription(cdlRcPtr->getDescription());
        float slope[3] = { 0.f, 0.f, 0.f };
        cdlRcPtr->getSlope(slope);
        cdl.setSlope(slope);
        float offset[3] = { 0.f, 0.f, 0.f };
        cdlRcPtr->getOffset(offset);
        cdl.setOffset(offset);
        float power[3] = { 0.f, 0.f, 0.f };
        cdlRcPtr->getPower(power);
        cdl.setPower(power);
        cdl.setSat(cdlRcPtr->getSat());
    }
    
    CDLTransformRcPtr CDLTransform::Create()
    {
        return CDLTransformRcPtr(new CDLTransform(), &deleter);
    }
    
    namespace
    {
        std::string GetCDLLocalCacheKey(const std::string & src, const std::string & cccid)
        {
            std::ostringstream os;
            os << src << " : " << cccid;
            return os.str();
        }
        
        std::string GetCDLLocalCacheKey(const char * src,
                                        int cccindex)
        {
            std::ostringstream os;
            os << src << " : " << cccindex;
            return os.str();
        }
        
        CDLTransformMap g_cache;
        StringBoolMap g_cacheSrcIsCC;
        Mutex g_cacheMutex;
    }
    
    void ClearCDLTransformFileCache()
    {
        AutoMutex lock(g_cacheMutex);
        g_cache.clear();
        g_cacheSrcIsCC.clear();
    }
    
    // TODO: Expose functions for introspecting in ccc file
    // TODO: Share caching with normal cdl pathway
    
    CDLTransformRcPtr CDLTransform::CreateFromFile(const char * src, const char * cccid_)
    {
        if(!src || (strlen(src) == 0) )
        {
            std::ostringstream os;
            os << "Error loading CDL xml. ";
            os << "Source file not specified.";
            throw Exception(os.str().c_str());
        }
        
        std::string cccid;
        if(cccid_) cccid = cccid_;
        
        // Check cache
        AutoMutex lock(g_cacheMutex);
        
        // Use g_cacheSrcIsCC as a proxy for if we have loaded this source
        // file already (in which case it must be in cache, or an error)
        
        StringBoolMap::iterator srcIsCCiter = g_cacheSrcIsCC.find(src);
        if(srcIsCCiter != g_cacheSrcIsCC.end())
        {
            // If the source file is known to be a pure ColorCorrection element,
            // null out the cccid so its ignored.
            if(srcIsCCiter->second) cccid = "";
            
            // Search for the cccid by name
            CDLTransformMap::iterator iter = 
                g_cache.find(GetCDLLocalCacheKey(src, cccid));
            if(iter != g_cache.end())
            {
                return iter->second;
            }
            
            // Search for cccid by index
            int cccindex=0;
            if(StringToInt(&cccindex, cccid.c_str(), true))
            {
                iter = g_cache.find(GetCDLLocalCacheKey(src, cccindex));
                if(iter != g_cache.end())
                {
                    return iter->second;
                }
            }
            
            std::ostringstream os;
            os << "The specified cccid/cccindex '" << cccid;
            os << "' could not be loaded from the src file '";
            os << src;
            os << "'.";
            throw Exception (os.str().c_str());
        }
        
        
        // Try to read all ccs from the file, into cache
        std::ifstream istream(src);
        if(istream.fail()) {
            std::ostringstream os;
            os << "Error could not read CDL source file '" << src;
            os << "'. Please verify the file exists and appropriate ";
            os << "permissions are set.";
            throw Exception (os.str().c_str());
        }
        
        CDLParser parser(src);
        parser.parse(istream);

        if (parser.isCC())
        {
            // Load a single ColorCorrection into the cache
            CDLTransformRcPtr cdl = CDLTransform::Create();
            parser.getCDLTransform(cdl);

            cccid = "";
            g_cacheSrcIsCC[src] = true;
            g_cache[GetCDLLocalCacheKey(src, cccid)] = cdl;
        }
        else if(parser.isCCC())
        {
            // Load all CCs from the ColorCorrectionCollection
            // into the cache
            CDLTransformMap transformMap;
            CDLTransformVec transformVec;
            parser.getCDLTransforms(transformMap, transformVec);
            
            if(transformVec.empty())
            {
                std::ostringstream os;
                os << "Error loading ccc xml. ";
                os << "No ColorCorrection elements found in file '";
                os << src << "'.";
                throw Exception(os.str().c_str());
            }
            
            g_cacheSrcIsCC[src] = false;
            
            // Add all by transforms to cache
            // First by index, then by id
            for(unsigned int i=0; i<transformVec.size(); ++i)
            {
                g_cache[GetCDLLocalCacheKey(src, i)] = transformVec[i];
            }
            
            for(CDLTransformMap::iterator iter = transformMap.begin();
                iter != transformMap.end();
                ++iter)
            {
                g_cache[GetCDLLocalCacheKey(src, iter->first)] = iter->second;
            }
        }
        
        // The all transforms should be in the cache.  Look it up, and try
        // to return it.
        {
            // Search for the cccid by name
            CDLTransformMap::iterator iter = 
                g_cache.find(GetCDLLocalCacheKey(src, cccid));
            if(iter != g_cache.end())
            {
                return iter->second;
            }
            
            // Search for cccid by index
            int cccindex=0;
            if(StringToInt(&cccindex, cccid.c_str(), true))
            {
                iter = g_cache.find(GetCDLLocalCacheKey(src, cccindex));
                if(iter != g_cache.end())
                {
                    return iter->second;
                }
            }
            
            std::ostringstream os;
            os << "The specified cccid/cccindex '" << cccid;
            os << "' could not be loaded from the src file '";
            os << src;
            os << "'.";
            throw Exception (os.str().c_str());
        }
    }
    
    void CDLTransform::deleter(CDLTransform* t)
    {
        delete t;
    }
    
    class CDLTransform::Impl : public CDLOpData
    {
    public:       
        Impl() 
            :   CDLOpData()
            ,   m_direction(TRANSFORM_DIR_FORWARD)
        {
        }

        Impl(const Impl &) = delete;

        ~Impl() { }
        
        Impl& operator= (const Impl & rhs)
        {
            if (this != &rhs)
            {
                m_direction = rhs.m_direction;
                CDLOpData::operator=(rhs);
            }

            return *this;
        }

        TransformDirection m_direction;

        mutable std::string m_xml;
    };
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    
    
    CDLTransform::CDLTransform()
        : m_impl(new CDLTransform::Impl)
    {
    }
    
    TransformRcPtr CDLTransform::createEditableCopy() const
    {
        CDLTransformRcPtr transform = CDLTransform::Create();
        *(transform->m_impl) = *m_impl;
        return transform;
    }
    
    CDLTransform::~CDLTransform()
    {
        delete m_impl;
        m_impl = NULL;
    }
    
    CDLTransform& CDLTransform::operator= (const CDLTransform & rhs)
    {
        if (this != &rhs)
        {
            *m_impl = *rhs.m_impl;
        }
        return *this;
    }
    
    TransformDirection CDLTransform::getDirection() const
    {
        return getImpl()->m_direction;
    }
    
    void CDLTransform::setDirection(TransformDirection dir)
    {
        getImpl()->m_direction = dir;
    }
    
    void CDLTransform::validate() const
    {
        try
        {
            Transform::validate();
            getImpl()->validate();
        }
        catch(Exception & ex)
        {
            std::string errMsg("CDLTransform validation failed: ");
            errMsg += ex.what();
            throw Exception(errMsg.c_str());
        }
    }

    const char * CDLTransform::getXML() const
    {
        getImpl()->m_xml = BuildXML(*this);
        return getImpl()->m_xml.c_str();
    }
    
    void CDLTransform::setXML(const char * xml)
    {
        LoadCDL(*this, xml);
    }
    
    // We use this approach, rather than comparing XML to get around the
    // case where setXML with extra data was provided.

    bool CDLTransform::equals(const ConstCDLTransformRcPtr & other) const
    {
        if(!other) return false;
        
        // NB: A tolerance of 1e-9 is used when comparing the parameters.
        return *(getImpl()) == *(other->getImpl())
            && getImpl()->m_direction == other->getImpl()->m_direction;
    }
    
    void CDLTransform::setSlope(const float * rgb)
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setSlopeParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
    }
    
    void CDLTransform::getSlope(float * rgb) const
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & params = getImpl()->getSlopeParams();
        rgb[0] = (float)params[0];
        rgb[1] = (float)params[1];
        rgb[2] = (float)params[2];
    }

    void CDLTransform::setOffset(const float * rgb)
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setOffsetParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
    }
    
    void CDLTransform::getOffset(float * rgb) const
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & params = getImpl()->getOffsetParams();
        rgb[0] = (float)params[0];
        rgb[1] = (float)params[1];
        rgb[2] = (float)params[2];
    }

    void CDLTransform::setPower(const float * rgb)
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setPowerParams(CDLOpData::ChannelParams(rgb[0], rgb[1], rgb[2]));
    }
    
    void CDLTransform::getPower(float * rgb) const
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & params = getImpl()->getPowerParams();
        rgb[0] = (float)params[0];
        rgb[1] = (float)params[1];
        rgb[2] = (float)params[2];
    }

    void CDLTransform::setSOP(const float * vec9)
    {
        if(!vec9)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        getImpl()->setSlopeParams(CDLOpData::ChannelParams(vec9[0], vec9[1], vec9[2]));
        getImpl()->setOffsetParams(CDLOpData::ChannelParams(vec9[3], vec9[4], vec9[5]));
        getImpl()->setPowerParams(CDLOpData::ChannelParams(vec9[6], vec9[7], vec9[8]));
    }
    
    void CDLTransform::getSOP(float * vec9) const
    {
        if(!vec9)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        const CDLOpData::ChannelParams & slopes = getImpl()->getSlopeParams();
        vec9[0] = (float)slopes[0];
        vec9[1] = (float)slopes[1];
        vec9[2] = (float)slopes[2];

        const CDLOpData::ChannelParams & offsets = getImpl()->getOffsetParams();
        vec9[3] = (float)offsets[0];
        vec9[4] = (float)offsets[1];
        vec9[5] = (float)offsets[2];

        const CDLOpData::ChannelParams & powers = getImpl()->getPowerParams();
        vec9[6] = (float)powers[0];
        vec9[7] = (float)powers[1];
        vec9[8] = (float)powers[2];
    }

    void CDLTransform::setSat(float sat)
    {
        getImpl()->setSaturation((double)sat);
    }
    
    float CDLTransform::getSat() const
    {
        return (float)getImpl()->getSaturation();
    }

    void CDLTransform::getSatLumaCoefs(float * rgb) const
    {
        if(!rgb)
        {
            throw Exception("CDLTransform: Invalid input pointer");
        }

        rgb[0] = 0.2126f;
        rgb[1] = 0.7152f;
        rgb[2] = 0.0722f;
    }
    
    void CDLTransform::setID(const char * id)
    {
        getImpl()->setID(id ? id : "");
    }
    
    const char * CDLTransform::getID() const
    {
        return getImpl()->getID().c_str();
    }
    
    void CDLTransform::setDescription(const char * desc)
    {
        getImpl()->setDescriptions(OpData::Descriptions(desc ? desc : ""));
    }
    
    const char * CDLTransform::getDescription() const
    {
        if(getImpl()->getDescriptions().empty())
            return "";

        return getImpl()->getDescriptions()[0].c_str();
    }
    
    std::ostream& operator<< (std::ostream& os, const CDLTransform& t)
    {
        float sop[9];
        t.getSOP(sop);
        
        os << "<CDLTransform";
        os << " direction=" << TransformDirectionToString(t.getDirection());
        os << ", sop=";
        for (unsigned int i=0; i<9; ++i)
        {
            if(i!=0) os << " ";
            os << sop[i];
        }
        os << ", sat=" << t.getSat();
        os << ">";
        return os;
    }
    
    
    ///////////////////////////////////////////////////////////////////////////
    
    void BuildCDLOps(OpRcPtrVec & ops,
                     const Config & config,
                     const CDLTransform & cdlTransform,
                     TransformDirection dir)
    {
        float scale4[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        cdlTransform.getSlope(scale4);
        
        float offset4[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        cdlTransform.getOffset(offset4);
        
        float power4[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        cdlTransform.getPower(power4);
        
        float lumaCoef3[] = { 1.0f, 1.0f, 1.0f };
        cdlTransform.getSatLumaCoefs(lumaCoef3);
        
        float sat = cdlTransform.getSat();
        
        TransformDirection combinedDir = CombineTransformDirections(dir,
                                                  cdlTransform.getDirection());
        
        if(config.getMajorVersion()==1)
        {
            const double p[4] = { power4[0], power4[1], power4[2], power4[3] };

            if(combinedDir == TRANSFORM_DIR_FORWARD)
            {
                // 1) Scale + Offset
                CreateScaleOffsetOp(ops, scale4, offset4, TRANSFORM_DIR_FORWARD);
                
                // 2) Power + Clamp at 0 (NB: This is not in accord with the 
                //    ASC v1.2 spec since it also requires clamping at 1.)
                CreateExponentOp(ops, p, TRANSFORM_DIR_FORWARD);
                
                // 3) Saturation (NB: Does not clamp at 0 and 1
                //    as per ASC v1.2 spec)
                CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_FORWARD);
            }
            else if(combinedDir == TRANSFORM_DIR_INVERSE)
            {
                // 3) Saturation (NB: Does not clamp at 0 and 1
                //    as per ASC v1.2 spec)
                CreateSaturationOp(ops, sat, lumaCoef3, TRANSFORM_DIR_INVERSE);
                
                // 2) Power + Clamp at 0 (NB: This is not in accord with the 
                //    ASC v1.2 spec since it also requires clamping at 1.)
                CreateExponentOp(ops, p, TRANSFORM_DIR_INVERSE);
                
                // 1) Scale + Offset
                CreateScaleOffsetOp(ops, scale4, offset4, TRANSFORM_DIR_INVERSE);
            }
        }
        else
        {
            // Starting with the version 2, OCIO is now using a CDL Op
            //   complying with the Common LUT Format (i.e. CLF) specification.

            const double s[3] = { scale4[0],  scale4[1],  scale4[2]  };
            const double o[3] = { offset4[0], offset4[1], offset4[2] };
            const double p[3] = { power4[0],  power4[1],  power4[2]  };

            CreateCDLOp(ops, 
                        cdlTransform.getID(),
                        OpData::Descriptions(cdlTransform.getDescription()),
                        combinedDir==TRANSFORM_DIR_FORWARD 
                            ? CDLOpData::CDL_V1_2_FWD
                            : CDLOpData::CDL_V1_2_REV,
                        s, o, p, double(sat), 
                        combinedDir);
        }
    }
}
OCIO_NAMESPACE_EXIT

#ifdef OCIO_UNIT_TEST

namespace OCIO = OCIO_NAMESPACE;
#include "UnitTest.h"
#include "UnitTestUtils.h"
#include "Platform.h"


OCIO_ADD_TEST(CDLTransform, equality)
{
    const OCIO::CDLTransformRcPtr cdl1 = OCIO::CDLTransform::Create();
    const OCIO::CDLTransformRcPtr cdl2 = OCIO::CDLTransform::Create();

    OCIO_CHECK_ASSERT(cdl1->equals(cdl1));
    OCIO_CHECK_ASSERT(cdl1->equals(cdl2));
    OCIO_CHECK_ASSERT(cdl2->equals(cdl1));

    const OCIO::CDLTransformRcPtr cdl3 = OCIO::CDLTransform::Create();
    cdl3->setSat(cdl3->getSat()+0.002f);

    OCIO_CHECK_ASSERT(!cdl1->equals(cdl3));
    OCIO_CHECK_ASSERT(!cdl2->equals(cdl3));
    OCIO_CHECK_ASSERT(cdl3->equals(cdl3));
}

OCIO_ADD_TEST(CDLTransform, CreateFromCCFile)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir())
                               + "/cdl_test1.cc");
    OCIO::CDLTransformRcPtr transform =
        OCIO::CDLTransform::CreateFromFile(filePath.c_str(), NULL);

    {
        std::string idStr(transform->getID());
        OCIO_CHECK_EQUAL("foo", idStr);
        std::string descStr(transform->getDescription());
        OCIO_CHECK_EQUAL("this is a description", descStr);
        float slope[3] = {0.f, 0.f, 0.f};
        OCIO_CHECK_NO_THROW(transform->getSlope(slope));
        OCIO_CHECK_EQUAL(1.1f, slope[0]);
        OCIO_CHECK_EQUAL(1.2f, slope[1]);
        OCIO_CHECK_EQUAL(1.3f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getOffset(offset));
        OCIO_CHECK_EQUAL(2.1f, offset[0]);
        OCIO_CHECK_EQUAL(2.2f, offset[1]);
        OCIO_CHECK_EQUAL(2.3f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getPower(power));
        OCIO_CHECK_EQUAL(3.1f, power[0]);
        OCIO_CHECK_EQUAL(3.2f, power[1]);
        OCIO_CHECK_EQUAL(3.3f, power[2]);
        OCIO_CHECK_EQUAL(0.7f, transform->getSat());
    }

    const std::string expectedOutXML(
        "<ColorCorrection id=\"foo\">\n"
        "    <SOPNode>\n"
        "        <Description>this is a description</Description>\n"
        "        <Slope>1.1 1.2 1.3</Slope>\n"
        "        <Offset>2.1 2.2 2.3</Offset>\n"
        "        <Power>3.1 3.2 3.3</Power>\n"
        "    </SOPNode>\n"
        "    <SatNode>\n"
        "        <Saturation>0.7</Saturation>\n"
        "    </SatNode>\n"
        "</ColorCorrection>");
    std::string outXML(transform->getXML());
    OCIO_CHECK_EQUAL(expectedOutXML, outXML);

    // parse again using setXML
    OCIO::CDLTransformRcPtr transformCDL = OCIO::CDLTransform::Create();
    transformCDL->setXML(expectedOutXML.c_str());
    {
        std::string idStr(transformCDL->getID());
        OCIO_CHECK_EQUAL("foo", idStr);
        std::string descStr(transformCDL->getDescription());
        OCIO_CHECK_EQUAL("this is a description", descStr);
        float slope[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transformCDL->getSlope(slope));
        OCIO_CHECK_EQUAL(1.1f, slope[0]);
        OCIO_CHECK_EQUAL(1.2f, slope[1]);
        OCIO_CHECK_EQUAL(1.3f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transformCDL->getOffset(offset));
        OCIO_CHECK_EQUAL(2.1f, offset[0]);
        OCIO_CHECK_EQUAL(2.2f, offset[1]);
        OCIO_CHECK_EQUAL(2.3f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transformCDL->getPower(power));
        OCIO_CHECK_EQUAL(3.1f, power[0]);
        OCIO_CHECK_EQUAL(3.2f, power[1]);
        OCIO_CHECK_EQUAL(3.3f, power[2]);
        OCIO_CHECK_EQUAL(0.7f, transformCDL->getSat());
    }
}

OCIO_ADD_TEST(CDLTransform, CreateFromCCCFile)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir())
                               + "/cdl_test1.ccc");
    {
        // Using ID
        OCIO::CDLTransformRcPtr transform =
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "cc0003");
        std::string idStr(transform->getID());
        OCIO_CHECK_EQUAL("cc0003", idStr);
        std::string descStr(transform->getDescription());
        OCIO_CHECK_EQUAL("golden", descStr);
        float slope[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getSlope(slope));
        OCIO_CHECK_EQUAL(1.2f, slope[0]);
        OCIO_CHECK_EQUAL(1.1f, slope[1]);
        OCIO_CHECK_EQUAL(1.0f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0f, offset[0]);
        OCIO_CHECK_EQUAL(0.0f, offset[1]);
        OCIO_CHECK_EQUAL(0.0f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getPower(power));
        OCIO_CHECK_EQUAL(0.9f, power[0]);
        OCIO_CHECK_EQUAL(1.0f, power[1]);
        OCIO_CHECK_EQUAL(1.2f, power[2]);
        OCIO_CHECK_EQUAL(1.0f, transform->getSat());
    }
    {
        // Using 0 based index
        OCIO::CDLTransformRcPtr transform =
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "3");
        std::string idStr(transform->getID());
        OCIO_CHECK_EQUAL("", idStr);
        std::string descStr(transform->getDescription());
        OCIO_CHECK_EQUAL("", descStr);
        float slope[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getSlope(slope));
        OCIO_CHECK_EQUAL(4.0f, slope[0]);
        OCIO_CHECK_EQUAL(5.0f, slope[1]);
        OCIO_CHECK_EQUAL(6.0f, slope[2]);
        float offset[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getOffset(offset));
        OCIO_CHECK_EQUAL(0.0f, offset[0]);
        OCIO_CHECK_EQUAL(0.0f, offset[1]);
        OCIO_CHECK_EQUAL(0.0f, offset[2]);
        float power[3] = { 0.f, 0.f, 0.f };
        OCIO_CHECK_NO_THROW(transform->getPower(power));
        OCIO_CHECK_EQUAL(0.9f, power[0]);
        OCIO_CHECK_EQUAL(1.0f, power[1]);
        OCIO_CHECK_EQUAL(1.2f, power[2]);
        OCIO_CHECK_EQUAL(1.0f, transform->getSat());
    }
}

OCIO_ADD_TEST(CDLTransform, CreateFromCCCFileFailure)
{
    const std::string filePath(std::string(OCIO::getTestFilesDir())
        + "/cdl_test1.ccc");
    {
        // Using ID
        OCIO_CHECK_THROW_WHAT(
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "NotFound"),
            OCIO::Exception, "could not be loaded from the src file");
    }
    {
        // Using index
        OCIO_CHECK_THROW_WHAT(
            OCIO::CDLTransform::CreateFromFile(filePath.c_str(), "42"),
            OCIO::Exception, "could not be loaded from the src file");
    }
}

OCIO_ADD_TEST(CDLTransform, EscapeXML)
{
    const std::string inputXML(
        "<ColorCorrection id=\"Esc &lt; &amp; &quot; &apos; &gt;\">\n"
        "    <SOPNode>\n"
        "        <Description>These: &lt; &amp; &quot; &apos; &gt; are escape chars</Description>\n"
        "        <Slope>1.1 1.2 1.3</Slope>\n"
        "        <Offset>2.1 2.2 2.3</Offset>\n"
        "        <Power>3.1 3.2 3.3</Power>\n"
        "    </SOPNode>\n"
        "    <SatNode>\n"
        "        <Saturation>0.7</Saturation>\n"
        "    </SatNode>\n"
        "</ColorCorrection>");

    // parse again using setXML
    OCIO::CDLTransformRcPtr transformCDL = OCIO::CDLTransform::Create();
    transformCDL->setXML(inputXML.c_str());
    {
        std::string idStr(transformCDL->getID());
        OCIO_CHECK_EQUAL("Esc < & \" ' >", idStr);
        std::string descStr(transformCDL->getDescription());
        OCIO_CHECK_EQUAL("These: < & \" ' > are escape chars", descStr);
    }
}

namespace
{
    static const std::string kContentsA = {
        "<ColorCorrectionCollection>\n"
        "    <ColorCorrection id=\"cc03343\">\n"
        "        <SOPNode>\n"
        "            <Slope>0.1 0.2 0.3 </Slope>\n"
        "            <Offset>0.8 0.1 0.3 </Offset>\n"
        "            <Power>0.5 0.5 0.5 </Power>\n"
        "        </SOPNode>\n"
        "        <SATNode>\n"
        "            <Saturation>1</Saturation>\n"
        "        </SATNode>\n"
        "    </ColorCorrection>\n"
        "    <ColorCorrection id=\"cc03344\">\n"
        "        <SOPNode>\n"
        "            <Slope>1.2 1.3 1.4 </Slope>\n"
        "            <Offset>0.3 0 0 </Offset>\n"
        "            <Power>0.75 0.75 0.75 </Power>\n"
        "        </SOPNode>\n"
        "        <SATNode>\n"
        "            <Saturation>1</Saturation>\n"
        "        </SATNode>\n"
        "    </ColorCorrection>\n"
        "</ColorCorrectionCollection>\n"
        };

    static const std::string kContentsB = {
        "<ColorCorrectionCollection>\n"
        "    <ColorCorrection id=\"cc03343\">\n"
        "        <SOPNode>\n"
        "            <Slope>1.1 2.2 3.3 </Slope>\n"
        "            <Offset>0.8 0.1 0.3 </Offset>\n"
        "            <Power>0.5 0.5 0.5 </Power>\n"
        "        </SOPNode>\n"
        "        <SATNode>\n"
        "            <Saturation>1</Saturation>\n"
        "        </SATNode>\n"
        "    </ColorCorrection>\n"
        "    <ColorCorrection id=\"cc03344\">\n"
        "        <SOPNode>\n"
        "            <Slope>1.2 1.3 1.4 </Slope>\n"
        "            <Offset>0.3 0 0 </Offset>\n"
        "            <Power>0.75 0.75 0.75 </Power>\n"
        "        </SOPNode>\n"
        "        <SATNode>\n"
        "            <Saturation>1</Saturation>\n"
        "        </SATNode>\n"
        "    </ColorCorrection>\n"
        "</ColorCorrectionCollection>\n"
        };


}

OCIO_ADD_TEST(CDLTransform, clear_caches)
{
    std::string filename;
    OCIO_CHECK_NO_THROW(OCIO::Platform::CreateTempFilename(filename, ""));

    std::fstream stream(filename, std::ios_base::out|std::ios_base::trunc);
    stream << kContentsA;
    stream.close();

    OCIO::CDLTransformRcPtr transform; 
    OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(filename.c_str(), "cc03343"));

    float slope[3];

    OCIO_CHECK_NO_THROW(transform->getSlope(slope));
    OCIO_CHECK_EQUAL(slope[0], 0.1f);
    OCIO_CHECK_EQUAL(slope[1], 0.2f);
    OCIO_CHECK_EQUAL(slope[2], 0.3f);

    stream.open(filename, std::ios_base::out|std::ios_base::trunc);
    stream << kContentsB;
    stream.close();

    OCIO_CHECK_NO_THROW(OCIO::ClearAllCaches());

    OCIO_CHECK_NO_THROW(transform = OCIO::CDLTransform::CreateFromFile(filename.c_str(), "cc03343"));
    OCIO_CHECK_NO_THROW(transform->getSlope(slope));

    OCIO_CHECK_EQUAL(slope[0], 1.1f);
    OCIO_CHECK_EQUAL(slope[1], 2.2f);
    OCIO_CHECK_EQUAL(slope[2], 3.3f);
}

OCIO_ADD_TEST(CDLTransform, faulty_file_content)
{
    std::string filename;
    OCIO_CHECK_NO_THROW(OCIO::Platform::CreateTempFilename(filename, ""));

    {
        std::fstream stream(filename, std::ios_base::out|std::ios_base::trunc);
        stream << kContentsA << "Some Extra faulty information";
        stream.close();

        OCIO_CHECK_THROW_WHAT(OCIO::CDLTransform::CreateFromFile(filename.c_str(), "cc03343"),
                              OCIO::Exception,
                              "Error parsing ColorCorrectionCollection (). Error is: XML parsing error");
    }
    {
        // Duplicated identifier.

        std::string faultyContent = kContentsA;
        const std::size_t found = faultyContent.find("cc03344");
        OCIO_CHECK_ASSERT(found!=std::string::npos);
        faultyContent.replace(found, strlen("cc03344"), "cc03343");


        std::fstream stream(filename, std::ios_base::out|std::ios_base::trunc);
        stream << faultyContent;
        stream.close();

        OCIO_CHECK_THROW_WHAT(OCIO::CDLTransform::CreateFromFile(filename.c_str(), "cc03343"),
                              OCIO::Exception,
                              "Error loading ccc xml. Duplicate elements with 'cc03343'");
    }
}
    
#endif
