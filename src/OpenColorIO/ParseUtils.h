// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_PARSEUTILS_H
#define INCLUDED_OCIO_PARSEUTILS_H

#include <sstream>
#include <string>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "PrivateTypes.h"

OCIO_NAMESPACE_ENTER
{
    // Prepares a string to be inserted in an XML document by escaping
    // characters that may not appear directly in XML.
    // (Note: eXpat does the inverse conversion automatically.)
    std::string ConvertSpecialCharToXmlToken(const std::string& str);

    std::string FloatToString(float fval);
    std::string FloatVecToString(const float * fval, unsigned int size);
    
    std::string DoubleToString(double value);
    std::string DoubleVecToString(const double * fval, unsigned int size);

    bool StringToFloat(float * fval, const char * str);
    bool StringToInt(int * ival, const char * str, bool failIfLeftoverChars=false);
    
    bool StringVecToFloatVec(std::vector<float> & floatArray,
                             const StringVec & lineParts);
    
    bool StringVecToIntVec(std::vector<int> & intArray,
                           const StringVec & lineParts);
    
    //////////////////////////////////////////////////////////////////////////
    
    // read the next non empty line, and store it in 'line'
    // return 'true' on success
    
    bool nextline(std::istream &istream, std::string &line);
    
    bool StrEqualsCaseIgnore(const std::string & a, const std::string & b);
    
    // If a ',' is in the string, split on it
    // If a ':' is in the string, split on it
    // Otherwise, assume a single string.
    // Also, strip whitespace from all parts.
    
    void SplitStringEnvStyle(StringVec & outputvec, const char * str);
    
    // Join on ','
    std::string JoinStringEnvStyle(const StringVec & outputvec);
    
    // Ordering and capitalization from vec1 is preserved
    StringVec IntersectStringVecsCaseIgnore(const StringVec & vec1,
                                            const StringVec & vec2);
    
    // Find the index of the specified string, ignoring case.
    // return -1 if not found.
    
    int FindInStringVecCaseIgnore(const StringVec & vec, const std::string & str);
    
}
OCIO_NAMESPACE_EXIT

#endif
