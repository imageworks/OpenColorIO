/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
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


#ifndef INCLUDED_OCIO_IMAGEPACKING_H
#define INCLUDED_OCIO_IMAGEPACKING_H

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"


OCIO_NAMESPACE_ENTER
{

struct GenericImageDesc
{
    long m_width  = 0;
    long m_height = 0;

    ptrdiff_t m_xStrideBytes = 0;
    ptrdiff_t m_yStrideBytes = 0;
    
    char * m_rData = nullptr;
    char * m_gData = nullptr;
    char * m_bData = nullptr;
    char * m_aData = nullptr;

    // Conversion op to/from 32-bit float to enforce float internal processing.
    ConstOpCPURcPtr m_bitDepthOp;

    // Is the image buffer a RGBA packed buffer?
    bool m_isRGBAPacked = false;
    // Is the image buffer a 32-bit float image buffer?
    bool m_isFloat      = false;

    
    // Resolves all AutoStride.
    void init(const ImageDesc & img, BitDepth bitDepth, const ConstOpCPURcPtr & bitDepthOp);
    
    // Is the image buffer a packed RGBA 32-bit float buffer?
    bool isPackedFloatRGBA() const;
    // Is the image buffer a RGBA packed buffer?
    bool isRGBAPacked() const;
    // Is the image buffer a 32-bit float image buffer?
    bool isFloat() const;
};

template<typename Type>
struct Generic
{
    static void PackRGBAFromImageDesc(const GenericImageDesc & srcImg,
                                      Type * inBitDepthBuffer,
                                      float * outputBuffer,
                                      int outputBufferSize,
                                      long imagePixelStartIndex);

    static void UnpackRGBAToImageDesc(GenericImageDesc & dstImg,
                                      float * inputBuffer,
                                      Type * outBitDepthBuffer,
                                      int numPixelsToUnpack,
                                      long imagePixelStartIndex);
};

}
OCIO_NAMESPACE_EXIT

#endif
