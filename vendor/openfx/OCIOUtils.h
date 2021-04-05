// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OFX_OCIOUTILS_H
#define INCLUDED_OFX_OCIOUTILS_H

#include <string>

#include "ofxsImageEffect.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

/* Get the current OCIO config */
OCIO::ConstConfigRcPtr getOCIOConfig();

/* Convert OFX bit-depth enum to OCIO bit-depth enum */
OCIO::BitDepth getOCIOBitDepth(OFX::BitDepthEnum ofxBitDepth);

/* Get number of bytes in a single pixel component at OCIO bit-depth */
int getChanStrideBytes(OCIO::BitDepth ocioBitDepth);

/* Build color space ChoiceParam from the current OCIO config */
OFX::ChoiceParamDescriptor * defineCsNameParam(
    OFX::ImageEffectDescriptor & desc,
    const std::string & name, 
    const std::string & label, 
    const std::string & hint,
    OFX::GroupParamDescriptor * parent);

/* Get current option string from a ChoiceParam */
std::string getChoiceParamOption(OFX::ChoiceParam * param);

#endif // INCLUDED_OFX_OCIOUTILS_H
