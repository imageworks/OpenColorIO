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


#ifndef INCLUDED_OCIO_OPDATATOOLS_H
#define INCLUDED_OCIO_OPDATATOOLS_H

#include <OpenColorIO/OpenColorIO.h>

#include "OpDataLut1D.h"
#include "OpDataInvLut1D.h"
#include "OpDataLut3D.h"


OCIO_NAMESPACE_ENTER
{
    // Private namespace to the OpData sub-directory
    namespace OpData
    {
        // Get the step size which is the smallest step between
        // two consecutive values in a LUT 1D or LUT 3D
        float GetValueStepSize(BitDepth bDepth, unsigned dimension);

        // Returns the ideal LUT size based on a specific bit depth
        unsigned GetLutIdealSize(BitDepth incomingBitDepth);

        // Control behavior of Lut1D composition.
        enum ComposeMethod
        {
            COMPOSE_RESAMPLE_NO      = 0, // Preserve original domain
            COMPOSE_RESAMPLE_INDEPTH = 1, // InDepth controls min size
            COMPOSE_RESAMPLE_BIG     = 2  // Min size is 65536
        };

        // Use functional composition to generate a single op that 
        // approximates the effect of the pair of ops.
        // 
        // IMPORTANT: Unless you really know what you're doing, you must call 
        // OpData::Lut1D::mayCompose before using this function.
        // 
        OpDataLut1DRcPtr Compose(const OpDataLut1DRcPtr & A, 
                                 const OpDataLut1DRcPtr & B, 
                                 ComposeMethod compFlag);

        // Use functional composition to generate a single op that 
        // approximates the effect of the pair of ops.
        OpDataLut3DRcPtr Compose(const OpDataLut3DRcPtr & A,
                                 const OpDataLut3DRcPtr & B);
    }
}
OCIO_NAMESPACE_EXIT

#endif
