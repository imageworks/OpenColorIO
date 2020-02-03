// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "BitDepthUtils.h"
#include "HashUtils.h"
#include "MathUtils.h"
#include "md5/md5.h"
#include "ops/lut3d/Lut3DOp.h"
#include "ops/lut3d/Lut3DOpData.h"
#include "ops/OpTools.h"
#include "ops/range/RangeOpData.h"
#include "Platform.h"

namespace OCIO_NAMESPACE
{

Lut3DOpDataRcPtr MakeFastLut3DFromInverse(ConstLut3DOpDataRcPtr & lut)
{
    if (lut->getDirection() != TRANSFORM_DIR_INVERSE)
    {
        throw Exception("MakeFastLut3DFromInverse expects an inverse LUT");
    }

    // TODO: The FastLut will limit inputs to [0,1].  If the forward LUT has an extended range
    // output, perhaps add a Range op before the FastLut to bring values into [0,1].

    // The composition needs to use the EXACT renderer.
    // (Also avoids infinite loop.)
    // So temporarily set the style to EXACT.
    LutStyleGuard<Lut3DOpData> guard(*lut);

    // Make a domain for the composed Lut3D.
    // TODO: Using a large number like 48 here is better for accuracy,
    // but it causes a delay when creating the renderer.
    const long GridSize = 48u;
    Lut3DOpDataRcPtr newDomain = std::make_shared<Lut3DOpData>(GridSize);

    newDomain->setFileOutputBitDepth(lut->getFileOutputBitDepth());

    // Compose the LUT newDomain with our inverse LUT (using INV_EXACT style).
    Lut3DOpData::Compose(newDomain, lut);

    // The INV_EXACT inversion style computes an inverse to the tetrahedral
    // style of forward evaluation.
    // TODO: Although this seems like the "correct" thing to do, it does
    // not seem to help accuracy (and is slower).  To investigate ...
    //newLut->setInterpolation(INTERP_TETRAHEDRAL);

    return newDomain;
}

// 129 allows for a MESH dimension of 7 in the 3dl file format.
const unsigned long Lut3DOpData::maxSupportedLength = 129;

// Functional composition is a concept from mathematics where two functions
// are combined into a single function.  This idea may be applied to ops
// where we generate a single op that has the same (or similar) effect as
// applying the two ops separately.  The motivation is faster processing.
//
// When composing LUTs, the algorithm produces a result which takes the
// domain of the first op into the range of the last op.  So the algorithm
// needs to render values through the ops.  In some cases the domain of
// the first op is sufficient, in other cases we need to create a new more
// finely sampled domain to try and make the result less lossy.
void Lut3DOpData::Compose(Lut3DOpDataRcPtr & A,
                          ConstLut3DOpDataRcPtr & B)
{
    // TODO: Composition of LUTs is a potentially lossy operation.
    // We try to be safe by making the result at least as big as either A or B
    // but we may want to even increase the resolution further.  However,
    // currently composition is done pairs at a time and we would want to
    // determine the increase size once at the start rather than bumping it up
    // as each pair is done.

    // We assume the caller has validated that A and B are forward LUTs.
    // TODO: Support inverse Lut3D here and refactor MakeFastLUT to use it.

    const long min_sz = B->getArray().getLength();
    const long n = A->getArray().getLength();
    OpRcPtrVec ops;

    Lut3DOpDataRcPtr domain;

    if (n >= min_sz)
    {
        // The range of the first LUT becomes the domain to interp in the second.
        // Use the original domain.
        domain = A;
    }
    else
    {
        // Since the 2nd LUT is more finely sampled, use its grid size.

        // Create identity with finer domain.

        // TODO: Should not need to create a new LUT object for this.
        //       Perhaps add a utility function to be shared with the constructor.

        auto metadata = A->getFormatMetadata();
        domain = std::make_shared<Lut3DOpData>(A->getInterpolation(), min_sz);

        domain->getFormatMetadata() = metadata;
        // Interpolate through both LUTs in this case (resample).
        CreateLut3DOp(ops, A, TRANSFORM_DIR_FORWARD);
    }

    // TODO: Would like to not require a clone simply to prevent the delete
    //       from being called on the op when the opList goes out of scope.
    Lut3DOpDataRcPtr clonedB = B->clone();
    CreateLut3DOp(ops, clonedB, TRANSFORM_DIR_FORWARD);


    const BitDepth fileOutBD = A->getFileOutputBitDepth();
    auto metadata = A->getFormatMetadata();

    // Min size, we replace it anyway.
    A = std::make_shared<Lut3DOpData>(A->getInterpolation(), 2);

    // TODO: May want to revisit metadata propagation.
    A->getFormatMetadata() = metadata;
    A->getFormatMetadata().combine(B->getFormatMetadata());

    A->setFileOutputBitDepth(fileOutBD);

    const Array::Values& inValues = domain->getArray().getValues();
    const long gridSize = domain->getArray().getLength();
    const long numPixels = gridSize * gridSize * gridSize;

    A->getArray().resize(gridSize, 3);
    Array::Values& outValues = A->getArray().getValues();

    EvalTransform((const float*)(&inValues[0]),
                  (float*)(&outValues[0]),
                  numPixels,
                  ops);
}

Lut3DOpData::Lut3DArray::Lut3DArray(unsigned long length)
{
    resize(length, getMaxColorComponents());
    fill();
}

Lut3DOpData::Lut3DArray::~Lut3DArray()
{
}

Lut3DOpData::Lut3DArray& Lut3DOpData::Lut3DArray::operator=(const Array& a)
{
    if (this != &a)
    {
        *(Array*)this = a;
    }
    return *this;
}

void Lut3DOpData::Lut3DArray::fill()
{
    // Make an identity LUT.
    const long length = getLength();
    const long maxChannels = getMaxColorComponents();

    Array::Values& values = getValues();

    const float stepValue = 1.0f / ((float)length - 1.0f);

    const long maxEntries = length*length*length;

    for (long idx = 0; idx<maxEntries; idx++)
    {
        values[maxChannels*idx + 0] = (float)((idx / length / length) % length) * stepValue;
        values[maxChannels*idx + 1] = (float)((idx / length) % length) * stepValue;
        values[maxChannels*idx + 2] = (float)(idx%length) * stepValue;
    }
}

void Lut3DOpData::Lut3DArray::resize(unsigned long length, unsigned long numColorComponents)
{
    if (length > maxSupportedLength)
    {
        std::ostringstream oss;
        oss << "LUT 3D: Grid size '" << length
            << "' must not be greater than '" << maxSupportedLength << "'.";
        throw Exception(oss.str().c_str());
    }
    Array::resize(length, numColorComponents);
}

unsigned long Lut3DOpData::Lut3DArray::getNumValues() const
{
    const unsigned long numEntries = getLength()*getLength()*getLength();
    return numEntries*getMaxColorComponents();
}

void Lut3DOpData::Lut3DArray::getRGB(long i, long j, long k, float* RGB) const
{
    const long length = getLength();
    const long maxChannels = getMaxColorComponents();
    const Array::Values& values = getValues();
    // Array order matches ctf order: channels vary most rapidly, then B, G, R.
    long offset = (i*length*length + j*length + k) * maxChannels;
    RGB[0] = values[offset];
    RGB[1] = values[++offset];
    RGB[2] = values[++offset];
}

void Lut3DOpData::Lut3DArray::setRGB(long i, long j, long k, float* RGB)
{
    const long length = getLength();
    const long maxChannels = getMaxColorComponents();
    Array::Values& values = getValues();
    // Array order matches ctf order: channels vary most rapidly, then B, G, R.
    long offset = (i*length*length + j*length + k) * maxChannels;
    values[offset] = RGB[0];
    values[++offset] = RGB[1];
    values[++offset] = RGB[2];
}

void Lut3DOpData::Lut3DArray::scale(float scaleFactor)
{
    // Don't scale if scaleFactor = 1.0f.
    if (scaleFactor != 1.0f)
    {
        Array::Values& arrayVals = getValues();
        const size_t size = arrayVals.size();

        for (size_t i = 0; i < size; i++)
        {
            arrayVals[i] *= scaleFactor;
        }
    }
}

Lut3DOpData::Lut3DOpData(unsigned long gridSize)
    : OpData()
    , m_interpolation(INTERP_DEFAULT)
    , m_array(gridSize)
    , m_direction(TRANSFORM_DIR_FORWARD)
    , m_invQuality(LUT_INVERSION_FAST)
{
}

Lut3DOpData::Lut3DOpData(long gridSize, TransformDirection dir)
    : OpData()
    , m_interpolation(INTERP_DEFAULT)
    , m_array(gridSize)
    , m_direction(dir)
    , m_invQuality(LUT_INVERSION_FAST)
{
}

Lut3DOpData::Lut3DOpData(Interpolation interpolation,
                         unsigned long gridSize)
    : OpData()
    , m_interpolation(interpolation)
    , m_array(gridSize)
    , m_direction(TRANSFORM_DIR_FORWARD)
    , m_invQuality(LUT_INVERSION_FAST)
{
}

Lut3DOpData::~Lut3DOpData()
{
}

void Lut3DOpData::setInterpolation(Interpolation algo)
{
    m_interpolation = algo;
}

Interpolation Lut3DOpData::getConcreteInterpolation() const
{
    switch (m_interpolation)
    {
    case INTERP_BEST:
    case INTERP_TETRAHEDRAL:
        return INTERP_TETRAHEDRAL;

    case INTERP_DEFAULT:
    case INTERP_LINEAR:
    case INTERP_CUBIC:
    case INTERP_NEAREST:
        // NB: In OCIO v2, INTERP_NEAREST is implemented as trilinear,
        // this is a change from OCIO v1.
    case INTERP_UNKNOWN:
        // NB: INTERP_UNKNOWN is not valid and will make validate() throw.
    default:
        return INTERP_LINEAR;
    }
}

void Lut3DOpData::setInversionQuality(LutInversionQuality style)
{
    m_invQuality = style;
}

void Lut3DOpData::setArrayFromRedFastestOrder(const std::vector<float> & lut)
{
    Array & lutArray = getArray();
    const auto lutSize = lutArray.getLength();

    if (lutSize * lutSize * lutSize * 3 != lut.size())
    {
        throw Exception("Lut3DOpData length does not match the vector size.");
    }

    for (unsigned long b = 0; b < lutSize; ++b)
    {
        for (unsigned long g = 0; g < lutSize; ++g)
        {
            for (unsigned long r = 0; r < lutSize; ++r)
            {
                // Lut3DOpData Array index. Blue changes fastest.
                const unsigned long blueFastIdx = 3 * ((r*lutSize + g)*lutSize + b);

                // Float array index. Red changes fastest.
                const unsigned long redFastIdx = 3 * ((b*lutSize + g)*lutSize + r);

                lutArray[blueFastIdx + 0] = lut[redFastIdx + 0];
                lutArray[blueFastIdx + 1] = lut[redFastIdx + 1];
                lutArray[blueFastIdx + 2] = lut[redFastIdx + 2];
            }
        }
    }
}

namespace
{
bool IsValid(const Interpolation & interpolation)
{
    switch (interpolation)
    {
    case INTERP_BEST:
    case INTERP_TETRAHEDRAL:
    case INTERP_DEFAULT:
    case INTERP_LINEAR:
    case INTERP_NEAREST:
        return true;
    case INTERP_CUBIC:
    case INTERP_UNKNOWN:
    default:
        return false;
    }
}
}

void Lut3DOpData::validate() const
{
    OpData::validate();

    if (!IsValid(m_interpolation))
    {
        throw Exception("Lut3D has an invalid interpolation type. ");
    }

    try
    {
        getArray().validate();
    }
    catch (Exception& e)
    {
        std::ostringstream oss;
        oss << "Lut3D content array issue: ";
        oss << e.what();

        throw Exception(oss.str().c_str());
    }

    if (getArray().getNumColorComponents() != 3)
    {
        throw Exception("Lut3D has an incorrect number of color components. ");
    }

    if (getArray().getLength()>maxSupportedLength)
    {
        // This should never happen. Enforced by resize.
        std::ostringstream oss;
        oss << "Lut3D length: " << getArray().getLength();
        oss << " is not supported. ";

        throw Exception(oss.str().c_str());
    }
}

bool Lut3DOpData::isNoOp() const
{
    // 3D LUT is clamping to its domain
    return false;
}

bool Lut3DOpData::isIdentity() const
{
    return false;
}

OpDataRcPtr Lut3DOpData::getIdentityReplacement() const
{
    return std::make_shared<RangeOpData>(0., 1., 0., 1.);
}

bool Lut3DOpData::haveEqualBasics(const Lut3DOpData & B) const
{
    // TODO: Should interpolation style be considered?
    return (m_array == B.m_array);
}

bool Lut3DOpData::operator==(const OpData & other) const
{
    if (!OpData::operator==(other)) return false;

    const Lut3DOpData* lop = static_cast<const Lut3DOpData*>(&other);

    // NB: The m_invQuality is not currently included.
    if (m_direction != lop->m_direction
        || m_interpolation != lop->m_interpolation)
    {
        return false;
    }

    return haveEqualBasics(*lop);
}

Lut3DOpDataRcPtr Lut3DOpData::clone() const
{
    return std::make_shared<Lut3DOpData>(*this);
}

bool Lut3DOpData::isInverse(ConstLut3DOpDataRcPtr & B) const
{
    if ((m_direction == TRANSFORM_DIR_FORWARD
         && B->m_direction == TRANSFORM_DIR_INVERSE) ||
        (m_direction == TRANSFORM_DIR_INVERSE
         && B->m_direction == TRANSFORM_DIR_FORWARD))
    {
        return haveEqualBasics(*B);
    }
    return false;
}

Lut3DOpDataRcPtr Lut3DOpData::inverse() const
{
    Lut3DOpDataRcPtr invLut = clone();

    invLut->m_direction = (m_direction == TRANSFORM_DIR_FORWARD) ?
                          TRANSFORM_DIR_INVERSE : TRANSFORM_DIR_FORWARD;

    // Note that any existing metadata could become stale at this point but
    // trying to update it is also challenging since inverse() is sometimes
    // called even during the creation of new ops.
    return invLut;
}

void Lut3DOpData::finalize()
{
    AutoMutex lock(m_mutex);

    validate();

    md5_state_t state;
    md5_byte_t digest[16];

    md5_init(&state);
    md5_append(&state,
               (const md5_byte_t *)&(getArray().getValues()[0]),
               (int)(getArray().getValues().size() * sizeof(float)));
    md5_finish(&state, digest);

    std::ostringstream cacheIDStream;
    if (!getID().empty())
    {
        cacheIDStream << getID() << " ";
    }

    cacheIDStream << GetPrintableHash(digest)                << " ";
    cacheIDStream << InterpolationToString(m_interpolation)  << " ";
    cacheIDStream << TransformDirectionToString(m_direction) << " ";

    // NB: The m_invQuality is not currently included.

    m_cacheID = cacheIDStream.str();
}

void Lut3DOpData::scale(float scale)
{
    getArray().scale(scale);
}

} // namespace OCIO_NAMESPACE

