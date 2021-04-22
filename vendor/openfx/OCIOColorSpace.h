// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OFX_OCIOCOLORSPACE_H
#define INCLUDED_OFX_OCIOCOLORSPACE_H

#include "ofxsImageEffect.h"

class OCIOColorSpace : public OFX::ImageEffect
{
protected:
    // Do not need to delete these. The ImageEffect is managing them for us.
    OFX::Clip *dstClip_;
    OFX::Clip *srcClip_;

    OFX::ChoiceParam * srcCsNameParam_;
    OFX::ChoiceParam * dstCsNameParam_;
    OFX::BooleanParam * inverseParam_;

public:
    OCIOColorSpace(OfxImageEffectHandle handle);

    /* Override the render */
    virtual void render(const OFX::RenderArguments & args);

    /* Override identity (~no-op) check */
    virtual bool isIdentity(const OFX::IsIdentityArguments & args, 
                            OFX::Clip *& identityClip, 
                            double & identityTime);

    /* Override changedParam */
    virtual void changedParam(const OFX::InstanceChangedArgs & args, 
                              const std::string & paramName);
};

mDeclarePluginFactory(OCIOColorSpaceFactory, {}, {});

#endif // INCLUDED_OFX_OCIOCOLORSPACE_H
