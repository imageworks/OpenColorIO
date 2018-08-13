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

#include "CTFMetadataElt.h"

OCIO_NAMESPACE_ENTER
{

// Private namespace to the OpData sub-directory
namespace CTF
{
// Private namespace for the xml reader utils
namespace Reader
{


MetadataElt::MetadataElt(const std::string& name,
                         ContainerElt* pParent,
                         unsigned xmlLineNumber,
                         const std::string& xmlFile)
    : ComplexElt(name, pParent, xmlLineNumber, xmlFile)
    , m_metadata(name)
{
}

MetadataElt::~MetadataElt()
{
}

void MetadataElt::start(const char ** atts)
{
    unsigned i = 0;
    while (atts[i] && *atts[i])
    {
        if (atts[i + 1] && *atts[i + 1])
        {
            m_metadata.addAttribute(OpData::Metadata::Attribute(atts[i], atts[i + 1]));
        }
        i += 2;
    }
}

void MetadataElt::end()
{
    MetadataElt* pMetadataElt = dynamic_cast<MetadataElt*>(getParent());
    if (pMetadataElt)
    {
        pMetadataElt->getMetadata()[getName()] = m_metadata;
    }
}

const std::string& MetadataElt::getIdentifier() const
{
    return getName();
}

void MetadataElt::setRawData(const char* str, size_t len, unsigned)
{
    m_metadata = m_metadata.getValue() + std::string(str, len);
}


} // exit Reader namespace
} // exit CTF namespace
}
OCIO_NAMESPACE_EXIT

