/*
Copyright (c) 2018 Autodesk Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <OpenColorIO/OpenColorIO.h>


namespace OCIO = OCIO_NAMESPACE;
#include "GPUUnitTest.h"

OCIO_NAMESPACE_USING

namespace
{

const int LUT3D_EDGE_SIZE = 32;

const double gamma[4]  = { 2.1,  2.2,  2.3,  1.5  };
const double offset[4] = {  .01,  .02,  .03,  .05 };


// Helper method to build unit tests.
void AddGammaTest(OCIOGPUTest & test, 
                  OCIO::GpuShaderDescRcPtr & shaderDesc,
                  TransformDirection direction,
                  const double * gamma,
                  const double * offset,
                  float epsilon)
{
    OCIO::GammaTransformRcPtr g = OCIO::GammaTransform::Create();
    g->setDirection(direction);
    g->setStyle(GAMMA_BASIC);
    g->setGamma(gamma);

    if(offset)
    {
        g->setStyle(GAMMA_MONCURVE);
        g->setOffset(offset);
    }

    test.setErrorThreshold(epsilon);

    test.setContext(g->createEditableCopy(), shaderDesc);
}

};

OCIO_ADD_GPU_TEST(GammaOp, basic_legacy_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddGammaTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, gamma, nullptr,
#ifdef USE_SSE
        5e-4f // Note: Related to the ssePower optimization !
#else
        5e-6f
#endif
        );
}


OCIO_ADD_GPU_TEST(GammaOp, basic)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddGammaTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, gamma, nullptr,
#ifdef USE_SSE
        5e-4f // Note: Related to the ssePower optimization !
#else
        5e-6f
#endif
        );
}


OCIO_ADD_GPU_TEST(GammaOp, basic_inverse_legacy_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddGammaTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, gamma, nullptr,
#ifdef USE_SSE
        5e-4f // Note: Related to the ssePower optimization !
#else
        1e-6f
#endif
        );
}


OCIO_ADD_GPU_TEST(GammaOp, basic_inverse)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddGammaTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, gamma, nullptr,
#ifdef USE_SSE
        1e-4f // Note: Related to the ssePower optimization !
#else
        1e-6f
#endif
        );
}


OCIO_ADD_GPU_TEST(GammaOp, moncurve_legacy_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddGammaTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, gamma, offset,
#ifdef USE_SSE
        1e-4f // Note: Related to the ssePower optimization !
#else
        5e-6f
#endif
        );
}


OCIO_ADD_GPU_TEST(GammaOp, moncurve_inverse_legacy_shader)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateLegacyShaderDesc(LUT3D_EDGE_SIZE);

    AddGammaTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, gamma, offset,
#ifdef USE_SSE
        5e-5f // Note: Related to the ssePower optimization !
#else
        5e-7f
#endif
        );
}


OCIO_ADD_GPU_TEST(GammaOp, moncurve)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddGammaTest(test, shaderDesc, TRANSFORM_DIR_FORWARD, gamma, offset,
#ifdef USE_SSE
        1e-4f // Note: Related to the ssePower optimization !
#else
        5e-6f
#endif
        );
}


OCIO_ADD_GPU_TEST(GammaOp, moncurve_inverse)
{
    OCIO::GpuShaderDescRcPtr shaderDesc 
        = OCIO::GpuShaderDesc::CreateShaderDesc();

    AddGammaTest(test, shaderDesc, TRANSFORM_DIR_INVERSE, gamma, offset,
#ifdef USE_SSE
        5e-5f // TODO: ssePower optimization !
#else
        5e-7f
#endif
        );
}


// Still need bit-depth coverage from these tests:
//      GPURendererGamma1_test
//      GPURendererGamma2_test
//      GPURendererGamma3_test
//      GPURendererGamma4_test
//      GPURendererGamma5_test
//      GPURendererGamma6_test
//      GPURendererGamma7_test
//      GPURendererGamma8_test

