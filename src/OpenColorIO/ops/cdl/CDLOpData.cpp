// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "ops/cdl/CDLOpData.h"
#include "ops/matrix/MatrixOp.h"
#include "ops/range/RangeOpData.h"
#include "Platform.h"


namespace OCIO_NAMESPACE
{

namespace DefaultValues
{
const int FLOAT_DECIMALS = 7;
}


static const CDLOpData::ChannelParams kOneParams(1.0);
static const CDLOpData::ChannelParams kZeroParams(0.0);

// Original CTF styles:
static const char V1_2_FWD_NAME[] = "v1.2_Fwd";
static const char V1_2_REV_NAME[] = "v1.2_Rev";
static const char NO_CLAMP_FWD_NAME[] = "noClampFwd";
static const char NO_CLAMP_REV_NAME[] = "noClampRev";

// CLF styles (also allowed now in CTF):
static const char V1_2_FWD_CLF_NAME[] = "Fwd";
static const char V1_2_REV_CLF_NAME[] = "Rev";
static const char NO_CLAMP_FWD_CLF_NAME[] = "FwdNoClamp";
static const char NO_CLAMP_REV_CLF_NAME[] = "RevNoClamp";

CDLOpData::Style CDLOpData::GetStyle(const char* name)
{

// Get the style enum  of the CDL from the name stored
// in the "name" variable.
#define RETURN_STYLE_FROM_NAME(CDL_STYLE_NAME, CDL_STYLE)  \
if( 0==Platform::Strcasecmp(name, CDL_STYLE_NAME) )        \
{                                                          \
return CDL_STYLE;                                          \
}

    if (name && *name)
    {
        RETURN_STYLE_FROM_NAME(V1_2_FWD_NAME, CDL_V1_2_FWD);
        RETURN_STYLE_FROM_NAME(V1_2_FWD_CLF_NAME, CDL_V1_2_FWD);
        RETURN_STYLE_FROM_NAME(V1_2_REV_NAME, CDL_V1_2_REV);
        RETURN_STYLE_FROM_NAME(V1_2_REV_CLF_NAME, CDL_V1_2_REV);
        RETURN_STYLE_FROM_NAME(NO_CLAMP_FWD_NAME, CDL_NO_CLAMP_FWD);
        RETURN_STYLE_FROM_NAME(NO_CLAMP_FWD_CLF_NAME, CDL_NO_CLAMP_FWD);
        RETURN_STYLE_FROM_NAME(NO_CLAMP_REV_NAME, CDL_NO_CLAMP_REV);
        RETURN_STYLE_FROM_NAME(NO_CLAMP_REV_CLF_NAME, CDL_NO_CLAMP_REV);
    }

#undef RETURN_STYLE_FROM_NAME

    throw Exception("Unknown style for CDL.");
}

const char * CDLOpData::GetStyleName(CDLOpData::Style style)
{
    // Get the name of the CDL style from the enum stored
    // in the "style" variable.
    switch (style)
    {
        case CDL_V1_2_FWD:     return V1_2_FWD_CLF_NAME;
        case CDL_V1_2_REV:     return V1_2_REV_CLF_NAME;
        case CDL_NO_CLAMP_FWD: return NO_CLAMP_FWD_CLF_NAME;
        case CDL_NO_CLAMP_REV: return NO_CLAMP_REV_CLF_NAME;
    }

    throw Exception("Unknown style for CDL.");
}

CDLOpData::CDLOpData()
    :   OpData()
    ,   m_style(GetDefaultStyle())
    ,   m_slopeParams(1.0)
    ,   m_offsetParams(0.0)
    ,   m_powerParams(1.0)
    ,   m_saturation(1.0)
{
}

CDLOpData::CDLOpData(const CDLOpData::Style & style,
                     const ChannelParams & slopeParams,
                     const ChannelParams & offsetParams,
                     const ChannelParams & powerParams,
                     const double saturation)
    :   OpData()
    ,   m_style(style)
    ,   m_slopeParams(slopeParams)
    ,   m_offsetParams(offsetParams)
    ,   m_powerParams(powerParams)
    ,   m_saturation(saturation)
{
    validate();
}

CDLOpData::~CDLOpData()
{
}

CDLOpDataRcPtr CDLOpData::clone() const
{
    return std::make_shared<CDLOpData>(*this);
}

bool CDLOpData::operator==(const OpData& other) const
{
    if (!OpData::operator==(other)) return false;

    const CDLOpData* cdl = static_cast<const CDLOpData*>(&other);

    return m_style        == cdl->m_style 
        && m_slopeParams  == cdl->m_slopeParams
        && m_offsetParams == cdl->m_offsetParams
        && m_powerParams  == cdl->m_powerParams
        && m_saturation   == cdl->m_saturation;
}

void CDLOpData::setStyle(const CDLOpData::Style & style)
{
    m_style = style;
}

void CDLOpData::setSlopeParams(const ChannelParams & slopeParams)
{
    m_slopeParams = slopeParams;
}

void CDLOpData::setOffsetParams(const ChannelParams & offsetParams)
{
    m_offsetParams = offsetParams;
}

void CDLOpData::setPowerParams(const ChannelParams & powerParams)
{
    m_powerParams = powerParams;
}

void CDLOpData::setSaturation(const double saturation)
{
    m_saturation = saturation;
}

// Validate if a parameter is greater than or equal to threshold value.
void validateGreaterEqual(const char * name, 
                          const double value, 
                          const double threshold)
{
    if (!(value >= threshold))
    {
        std::ostringstream oss;
        oss << "CDL: Invalid '";
        oss << name;
        oss << "' " << value;
        oss << " should be greater than ";
        oss << threshold << ".";
        throw Exception(oss.str().c_str());
    }
}

// Validate if a parameter is greater than a threshold value.
void validateGreaterThan(const char * name, 
                        const double value, 
                         const double threshold)
{ 
    if (!(value > threshold))
    {
        std::ostringstream oss;
        oss << "CDLOpData: Invalid '";
        oss << name;
        oss << "' " << value;
        oss << " should be greater than ";
        oss << threshold << ".";
        throw Exception(oss.str().c_str());
    }
}

typedef void(*parameter_validation_function)(const char *, 
                                             const double, 
                                             const double);

template<parameter_validation_function fnVal>
void validateChannelParams(const char * name, 
                           const CDLOpData::ChannelParams& params,
                           double threshold)
{
    for (unsigned i = 0; i < 3; ++i)
    {
        fnVal(name, params[i], threshold);
    }
}

// Validate number of SOP parameters and saturation.
// The ASC v1.2 spec 2009-05-04 places the following restrictions:
//   slope >= 0, power > 0, sat >= 0, (offset unbounded).
void validateParams(const CDLOpData::ChannelParams& slopeParams,
                    const CDLOpData::ChannelParams& powerParams,
                    const double saturation)
{
    // slope >= 0
    validateChannelParams<validateGreaterEqual>("slope", slopeParams, 0.0);

    // power > 0
    validateChannelParams<validateGreaterThan>("power", powerParams, 0.0);

    // saturation >= 0
    validateGreaterEqual("saturation", saturation, 0.0);
}

bool CDLOpData::isNoOp() const
{
    return isIdentity()
        && !isClamping();
}

bool CDLOpData::isIdentity() const
{
    return  m_slopeParams  == kOneParams  &&
            m_offsetParams == kZeroParams &&
            m_powerParams  == kOneParams  &&
            m_saturation   == 1.0;
}

OpDataRcPtr CDLOpData::getIdentityReplacement() const
{
    OpDataRcPtr op;
    switch(getStyle())
    {
        // These clamp values below 0 -- replace with range.
        case CDL_V1_2_FWD:
        case CDL_V1_2_REV:
        {
            op = std::make_shared<RangeOpData>(0.,
                                               RangeOpData::EmptyValue(), // don't clamp high end
                                               0.,
                                               RangeOpData::EmptyValue());
            break;
        }

        // These pass through the full range of values -- replace with matrix.
        case CDL_NO_CLAMP_FWD:
        case CDL_NO_CLAMP_REV:
        {
            op = std::make_shared<MatrixOpData>();
            break;
        }
    }
    return op;
}

bool CDLOpData::hasChannelCrosstalk() const
{
    return m_saturation != 1.0;
}

void CDLOpData::validate() const
{
    OpData::validate();

    validateParams(m_slopeParams, m_powerParams, m_saturation);
}

std::string CDLOpData::getSlopeString() const
{
    return GetChannelParametersString(m_slopeParams);
}

std::string CDLOpData::getOffsetString() const
{
    return GetChannelParametersString(m_offsetParams);
}

std::string CDLOpData::getPowerString() const
{
    return GetChannelParametersString(m_powerParams);
}

std::string CDLOpData::getSaturationString() const
{
    std::ostringstream oss;
    oss.precision(DefaultValues::FLOAT_DECIMALS);
    oss << m_saturation;
    return oss.str();
}

bool CDLOpData::isReverse() const
{
    const CDLOpData::Style style = getStyle();
    switch (style)
    {
        case CDLOpData::CDL_V1_2_FWD:     return false;
        case CDLOpData::CDL_V1_2_REV:     return true;
        case CDLOpData::CDL_NO_CLAMP_FWD: return false;
        case CDLOpData::CDL_NO_CLAMP_REV: return true;
    }
    return false;
}

bool CDLOpData::isClamping() const
{
    const CDLOpData::Style style = getStyle();
    switch (style)
    {
        case CDLOpData::CDL_V1_2_FWD:     return true;
        case CDLOpData::CDL_V1_2_REV:     return true;
        case CDLOpData::CDL_NO_CLAMP_FWD: return false;
        case CDLOpData::CDL_NO_CLAMP_REV: return false;
    }
    return false;
}

std::string CDLOpData::GetChannelParametersString(ChannelParams params)
{
    std::ostringstream oss;
    oss.precision(DefaultValues::FLOAT_DECIMALS);
    oss << params[0] << ", " << params[1] << ", " << params[2];
    return oss.str();
}

bool CDLOpData::isInverse(ConstCDLOpDataRcPtr & r) const
{
    // TODO: We are not detecting the case where you have two transforms with the same direction
    // but the parameters are inverses, e.g. offset of 0.1 and then -0.1
    return *r == *inverse();
}

CDLOpDataRcPtr CDLOpData::inverse() const
{
    CDLOpDataRcPtr cdl = clone();

    switch(cdl->getStyle())
    {
        case CDL_V1_2_FWD: cdl->setStyle(CDL_V1_2_REV); break;
        case CDL_V1_2_REV: cdl->setStyle(CDL_V1_2_FWD); break;
        case CDL_NO_CLAMP_FWD: cdl->setStyle(CDL_NO_CLAMP_REV); break;
        case CDL_NO_CLAMP_REV: cdl->setStyle(CDL_NO_CLAMP_FWD); break;
    }

    // Note that any existing metadata could become stale at this point but
    // trying to update it is also challenging since inverse() is sometimes
    // called even during the creation of new ops.
    return cdl;
}

void CDLOpData::finalize()
{
    AutoMutex lock(m_mutex);

    validate();

    std::ostringstream cacheIDStream;
    cacheIDStream << getID() << " ";

    cacheIDStream.precision(DefaultValues::FLOAT_DECIMALS);

    cacheIDStream << GetStyleName(getStyle()) << " ";
    cacheIDStream << getSlopeString() << " ";
    cacheIDStream << getOffsetString() << " ";
    cacheIDStream << getPowerString() << " ";
    cacheIDStream << getSaturationString() << " ";

    m_cacheID = cacheIDStream.str();
}

} // namespace OCIO_NAMESPACE

