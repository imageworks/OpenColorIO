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

#include <sstream>
#include <typeinfo>

#include <OpenColorIO/OpenColorIO.h>

#include "fileformats/FormatMetadata.h"
#include "OpBuilders.h"
#include "ops/CDL/CDLOps.h"
#include "ops/Exponent/ExponentOps.h"
#include "ops/exposurecontrast/ExposureContrastOps.h"
#include "ops/FixedFunction/FixedFunctionOps.h"
#include "ops/Gamma/GammaOps.h"
#include "ops/Log/LogOps.h"
#include "ops/Lut1D/Lut1DOp.h"
#include "ops/Lut3D/Lut3DOp.h"
#include "ops/Matrix/MatrixOps.h"
#include "ops/Range/RangeOps.h"
#include "Processor.h"
#include "TransformBuilder.h"

OCIO_NAMESPACE_ENTER
{
    Transform::~Transform()
    { }

    void Transform::validate() const
    { 
        if (getDirection() != TRANSFORM_DIR_FORWARD
            && getDirection() != TRANSFORM_DIR_INVERSE)
        {
            std::string err(typeid(*this).name());
            err += ": invalid direction";

            throw Exception(err.c_str());
        }
    }

    void BuildOps(OpRcPtrVec & ops,
                  const Config & config,
                  const ConstContextRcPtr & context,
                  const ConstTransformRcPtr & transform,
                  TransformDirection dir)
    {
        // A null transform is valid, and corresponds to a no-op.
        if(!transform)
            return;
        
        if(ConstAllocationTransformRcPtr allocationTransform = \
            DynamicPtrCast<const AllocationTransform>(transform))
        {
            BuildAllocationOps(ops, config, *allocationTransform, dir);
        }
        else if(ConstCDLTransformRcPtr cdlTransform = \
            DynamicPtrCast<const CDLTransform>(transform))
        {
            BuildCDLOps(ops, config, *cdlTransform, dir);
        }
        else if(ConstColorSpaceTransformRcPtr colorSpaceTransform = \
            DynamicPtrCast<const ColorSpaceTransform>(transform))
        {
            BuildColorSpaceOps(ops, config, context, *colorSpaceTransform, dir);
        }
        else if(ConstDisplayTransformRcPtr displayTransform = \
            DynamicPtrCast<const DisplayTransform>(transform))
        {
            BuildDisplayOps(ops, config, context, *displayTransform, dir);
        }
        else if(ConstExponentTransformRcPtr exponentTransform = \
            DynamicPtrCast<const ExponentTransform>(transform))
        {
            BuildExponentOps(ops, config, *exponentTransform, dir);
        }
        else if(ConstExponentWithLinearTransformRcPtr expWithLinearTransform = \
            DynamicPtrCast<const ExponentWithLinearTransform>(transform))
        {
            BuildExponentWithLinearOps(ops, config, *expWithLinearTransform, dir);
        }
        else if (ConstExposureContrastTransformRcPtr ecTransform = \
            DynamicPtrCast<const ExposureContrastTransform>(transform))
        {
            BuildExposureContrastOps(ops, config, *ecTransform, dir);
        }
        else if(ConstFixedFunctionTransformRcPtr fixedFunctionTransform = \
            DynamicPtrCast<const FixedFunctionTransform>(transform))
        {
            BuildFixedFunctionOps(ops, config, context, *fixedFunctionTransform, dir);
        }
        else if(ConstFileTransformRcPtr fileTransform = \
            DynamicPtrCast<const FileTransform>(transform))
        {
            BuildFileTransformOps(ops, config, context, *fileTransform, dir);
        }
        else if(ConstGroupTransformRcPtr groupTransform = \
            DynamicPtrCast<const GroupTransform>(transform))
        {
            BuildGroupOps(ops, config, context, *groupTransform, dir);
        }
        else if(ConstLogAffineTransformRcPtr logAffineTransform = \
            DynamicPtrCast<const LogAffineTransform>(transform))
        {
            BuildLogOps(ops, config, *logAffineTransform, dir);
        }
        else if(ConstLogTransformRcPtr logTransform = \
            DynamicPtrCast<const LogTransform>(transform))
        {
            BuildLogOps(ops, config, *logTransform, dir);
        }
        else if(ConstLookTransformRcPtr lookTransform = \
            DynamicPtrCast<const LookTransform>(transform))
        {
            BuildLookOps(ops, config, context, *lookTransform, dir);
        }
        else if (ConstLUT1DTransformRcPtr lut1dTransform = \
            DynamicPtrCast<const LUT1DTransform>(transform))
        {
            BuildLut1DOps(ops, config, *lut1dTransform, dir);
        }
        else if (ConstLUT3DTransformRcPtr lut1dTransform = \
            DynamicPtrCast<const LUT3DTransform>(transform))
        {
            BuildLut3DOps(ops, config, *lut1dTransform, dir);
        }
        else if(ConstMatrixTransformRcPtr matrixTransform = \
            DynamicPtrCast<const MatrixTransform>(transform))
        {
            BuildMatrixOps(ops, config, *matrixTransform, dir);
        }
        else if(ConstRangeTransformRcPtr rangeTransform = \
            DynamicPtrCast<const RangeTransform>(transform))
        {
            BuildRangeOps(ops, config, *rangeTransform, dir);
        }
        else
        {
            std::ostringstream error;
            error << "Unknown transform type for creation: "
                  << typeid(transform).name();

            throw Exception(error.str().c_str());
        }
    }
    
    std::ostream& operator<< (std::ostream & os, const Transform & transform)
    {
        const Transform* t = &transform;
        
        if(const AllocationTransform * allocationTransform = \
            dynamic_cast<const AllocationTransform*>(t))
        {
            os << *allocationTransform;
        }
        else if(const CDLTransform * cdlTransform = \
            dynamic_cast<const CDLTransform*>(t))
        {
            os << *cdlTransform;
        }
        else if(const ColorSpaceTransform * colorSpaceTransform = \
            dynamic_cast<const ColorSpaceTransform*>(t))
        {
            os << *colorSpaceTransform;
        }
        else if(const DisplayTransform * displayTransform = \
            dynamic_cast<const DisplayTransform*>(t))
        {
            os << *displayTransform;
        }
        else if(const ExponentTransform * exponentTransform = \
            dynamic_cast<const ExponentTransform*>(t))
        {
            os << *exponentTransform;
        }
        else if (const ExposureContrastTransform * ecTransform = \
            dynamic_cast<const ExposureContrastTransform*>(t))
        {
            os << *ecTransform;
        }
        else if(const FileTransform * fileTransform = \
            dynamic_cast<const FileTransform*>(t))
        {
            os << *fileTransform;
        }
        else if(const FixedFunctionTransform * fixedFunctionTransform = \
            dynamic_cast<const FixedFunctionTransform*>(t))
        {
            os << *fixedFunctionTransform;
        }
        else if(const GroupTransform * groupTransform = \
            dynamic_cast<const GroupTransform*>(t))
        {
            os << *groupTransform;
        }
        else if(const LogTransform * logTransform = \
            dynamic_cast<const LogTransform*>(t))
        {
            os << *logTransform;
        }
        else if(const LookTransform * lookTransform = \
            dynamic_cast<const LookTransform*>(t))
        {
            os << *lookTransform;
        }
        else if (const LUT1DTransform * lut1dTransform = \
            dynamic_cast<const LUT1DTransform*>(t))
        {
            os << *lut1dTransform;
        }
        else if (const LUT3DTransform * lut3dTransform = \
            dynamic_cast<const LUT3DTransform*>(t))
        {
            os << *lut3dTransform;
        }
        else if(const MatrixTransform * matrixTransform = \
            dynamic_cast<const MatrixTransform*>(t))
        {
            os << *matrixTransform;
        }
        else if(const RangeTransform * rangeTransform = \
            dynamic_cast<const RangeTransform*>(t))
        {
            os << *rangeTransform;
        }
        else
        {
            std::ostringstream error;
            error << "Unknown transform type for serialization: "
                  << typeid(transform).name();

            throw Exception(error.str().c_str());

        }
        
        return os;
    }


    void CreateTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op)
    {
        // AllocationNoOp, FileNoOp, LookNoOp won't create a Transform.
        if (!op || op->isNoOp())
            return;

        auto data = op->data();

        if (DynamicPtrCast<const CDLOpData>(data))
        {
            CreateCDLTransform(group, op);
        }
        else if (DynamicPtrCast<const ExponentOpData>(data))
        {
            CreateExponentTransform(group, op);
        }
        else if (DynamicPtrCast<const ExposureContrastOpData>(data))
        {
            CreateExposureContrastTransform(group, op);
        }
        else if (DynamicPtrCast<const FixedFunctionOpData>(data))
        {
            CreateFixedFunctionTransform(group, op);
        }
        else if (DynamicPtrCast<const GammaOpData>(data))
        {
            CreateGammaTransform(group, op);
        }
        else if (DynamicPtrCast<const LogOpData>(data))
        {
            CreateLogTransform(group, op);
        }
        else if (DynamicPtrCast<const Lut1DOpData>(data))
        {
            CreateLut1DTransform(group, op);
        }
        else if (DynamicPtrCast<const Lut3DOpData>(data))
        {
            CreateLut3DTransform(group, op);
        }
        else if (DynamicPtrCast<const MatrixOpData>(data))
        {
            CreateMatrixTransform(group, op);
        }
        else if (DynamicPtrCast<const RangeOpData>(data))
        {
            CreateRangeTransform(group, op);
        }
        else
        {
            std::ostringstream error;
            error << "CreateTransform from op. Missing implementation for: "
                  <<  typeid(op).name();

            throw Exception(error.str().c_str());
        }
    }


}
OCIO_NAMESPACE_EXIT
