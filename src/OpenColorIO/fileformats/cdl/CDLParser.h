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

#ifndef INCLUDED_OCIO_FILEFORMATS_CDL_CDLPARSER_H
#define INCLUDED_OCIO_FILEFORMATS_CDL_CDLPARSER_H

#include <string>
#include <istream>

#include <OpenColorIO/OpenColorIO.h>

#include "transforms/CDLTransform.h"

OCIO_NAMESPACE_ENTER
{

class FormatMetadataImpl;

class CDLParser
{
public:
    CDLParser(const std::string& xmlFile);
    virtual ~CDLParser();

    void parse(std::istream & istream) const;

    // Can be called after parse
    void getCDLTransforms(CDLTransformMap & transformMap,
                          CDLTransformVec & transformVec,
                          FormatMetadataImpl & metadata) const;

    // Can be called after parse
    void getCDLTransform(CDLTransformRcPtr & transform) const;

    bool isCC() const;
    bool isCCC() const;

private:
    // The hidden implementation's declaration
    class Impl;
    Impl * m_impl; 

    CDLParser() = delete;
    CDLParser(const CDLParser&) = delete;
    CDLParser& operator=(const CDLParser&) = delete;
};


}
OCIO_NAMESPACE_EXIT

#endif
