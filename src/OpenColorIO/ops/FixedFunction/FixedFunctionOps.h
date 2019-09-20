// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_FIXEDFUNCTION_H
#define INCLUDED_OCIO_FIXEDFUNCTION_H


#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "Op.h"
#include "ops/FixedFunction/FixedFunctionOpData.h"


OCIO_NAMESPACE_ENTER
{

void CreateFixedFunctionOp(OpRcPtrVec & ops, 
                           const FixedFunctionOpData::Params & data,
                           FixedFunctionOpData::Style style);

void CreateFixedFunctionOp(OpRcPtrVec & ops,
                           FixedFunctionOpDataRcPtr & funcData,
                           TransformDirection direction);

// Create a copy of the fixed function transform in the op and append it to
// the GroupTransform.
void CreateFixedFunctionTransform(GroupTransformRcPtr & group, ConstOpRcPtr & op);

}
OCIO_NAMESPACE_EXIT

#endif
