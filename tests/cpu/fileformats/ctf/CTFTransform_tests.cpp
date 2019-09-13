/*
Copyright (c) 2019 Autodesk Inc., et al.
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

#include "fileformats/ctf/CTFTransform.cpp"
#include "ops/Matrix/MatrixOpData.h"
#include "transforms/FileTransform.h"
#include "UnitTest.h"
#include "UnitTestUtils.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(CTFVersion, read_version)
{
    {
        const OCIO::CTFVersion version1(1, 2, 3);
        const OCIO::CTFVersion version2(1, 2, 3);
        OCIO_CHECK_EQUAL(version1, version2);
        {
            const OCIO::CTFVersion version3(0, 0, 1);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(0, 1, 0);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(1, 0, 0);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(1, 2, 0);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
        {
            const OCIO::CTFVersion version3(1, 2, 2);
            OCIO_CHECK_ASSERT(false == (version1 == version3));
            OCIO_CHECK_ASSERT(version3 < version1);
        }
    }

    OCIO::CTFVersion versionRead;
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.2.3", versionRead));
        const OCIO::CTFVersion version(1, 2, 3);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.2", versionRead));
        const OCIO::CTFVersion version(1, 2, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1", versionRead));
        const OCIO::CTFVersion version(1, 0, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.10", versionRead));
        const OCIO::CTFVersion version(1, 10, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.1.0", versionRead));
        const OCIO::CTFVersion version(1, 1, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }
    {
        OCIO_CHECK_NO_THROW(OCIO::CTFVersion::ReadVersion("1.01", versionRead));
        const OCIO::CTFVersion version(1, 1, 0);
        OCIO_CHECK_EQUAL(version, versionRead);
    }

    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("", versionRead),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1 2", versionRead),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1-2", versionRead),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("a", versionRead),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1.", versionRead),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion(".2", versionRead),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("1.0 2", versionRead),
        OCIO::Exception,
        "is not a valid version");
    OCIO_CHECK_THROW_WHAT(OCIO::CTFVersion::ReadVersion("-1", versionRead),
        OCIO::Exception,
        "is not a valid version");
}

OCIO_ADD_TEST(CTFVersion, version_write)
{
    {
        const OCIO::CTFVersion version(1, 2, 3);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.2.3");
    }
    {
        const OCIO::CTFVersion version(1, 0, 3);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.0.3");
    }
    {
        const OCIO::CTFVersion version(1, 2, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.2");
    }
    {
        const OCIO::CTFVersion version(1, 20, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1.20");
    }
    {
        const OCIO::CTFVersion version(1, 0, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "1");
    }
    {
        const OCIO::CTFVersion version(0, 0, 0);
        std::ostringstream ostream;
        ostream << version;
        OCIO_CHECK_EQUAL(ostream.str(), "0");
    }
}

OCIO_ADD_TEST(CTFReaderTransform, accessors)
{
    OCIO::CTFReaderTransform t;
    {
        const OCIO::CTFReaderTransform & ct = t;

        OCIO::FormatMetadataImpl & info = t.getInfoMetadata();
        const OCIO::FormatMetadataImpl & cinfo = t.getInfoMetadata();

        OCIO_CHECK_EQUAL(std::string(info.getName()), OCIO::METADATA_INFO);
        OCIO_CHECK_EQUAL(std::string(cinfo.getName()), OCIO::METADATA_INFO);

        OCIO_CHECK_EQUAL(t.getID(), "");
        OCIO_CHECK_EQUAL(ct.getID(), "");
        OCIO_CHECK_EQUAL(t.getName(), "");
        OCIO_CHECK_EQUAL(ct.getName(), "");
        OCIO_CHECK_EQUAL(t.getInverseOfId(), "");
        OCIO_CHECK_EQUAL(ct.getInverseOfId(), "");
        OCIO_CHECK_EQUAL(t.getInputDescriptor(), "");
        OCIO_CHECK_EQUAL(ct.getInputDescriptor(), "");
        OCIO_CHECK_EQUAL(t.getOutputDescriptor(), "");
        OCIO_CHECK_EQUAL(ct.getOutputDescriptor(), "");

        OCIO_CHECK_ASSERT(t.getOps().empty());
        OCIO_CHECK_ASSERT(ct.getOps().empty());

        OCIO_CHECK_ASSERT(t.getDescriptions().empty());
        OCIO_CHECK_ASSERT(ct.getDescriptions().empty());
    }
    t.setName("Name");
    t.setID("123");
    t.setInverseOfId("654");
    t.setInputDescriptor("input");
    t.setOutputDescriptor("output");

    auto matrixOp = std::make_shared<OCIO::MatrixOpData>();
    t.getOps().push_back(matrixOp);

    t.getDescriptions().push_back("One");
    t.getDescriptions().push_back("Two");

    {
        const OCIO::CTFReaderTransform & ct = t;

        OCIO_CHECK_EQUAL(t.getID(), "123");
        OCIO_CHECK_EQUAL(ct.getID(), "123");
        OCIO_CHECK_EQUAL(t.getName(), "Name");
        OCIO_CHECK_EQUAL(ct.getName(), "Name");
        OCIO_CHECK_EQUAL(t.getInverseOfId(), "654");
        OCIO_CHECK_EQUAL(ct.getInverseOfId(), "654");
        OCIO_CHECK_EQUAL(t.getInputDescriptor(), "input");
        OCIO_CHECK_EQUAL(ct.getInputDescriptor(), "input");
        OCIO_CHECK_EQUAL(t.getOutputDescriptor(), "output");
        OCIO_CHECK_EQUAL(ct.getOutputDescriptor(), "output");

        OCIO_CHECK_EQUAL(t.getOps().size(), 1);
        OCIO_CHECK_EQUAL(ct.getOps().size(), 1);

        OCIO_CHECK_EQUAL(t.getDescriptions().size(), 2);
        OCIO_CHECK_EQUAL(ct.getDescriptions().size(), 2);
        OCIO_CHECK_EQUAL(t.getDescriptions()[0], "One");
        OCIO_CHECK_EQUAL(ct.getDescriptions()[0], "One");
        OCIO_CHECK_EQUAL(t.getDescriptions()[1], "Two");
        OCIO_CHECK_EQUAL(ct.getDescriptions()[1], "Two");
    }
}

OCIO_ADD_TEST(CTFReaderTransform, validate_bitdepth_agreement)
{
    OCIO::CTFReaderTransform t;
    auto matrix = std::make_shared<OCIO::MatrixOpData>(OCIO::BIT_DEPTH_UINT10,
        OCIO::BIT_DEPTH_F32);
    t.getOps().push_back(matrix);

    matrix = std::make_shared<OCIO::MatrixOpData>(OCIO::BIT_DEPTH_F32,
        OCIO::BIT_DEPTH_F32);

    t.getOps().push_back(matrix);

    OCIO_CHECK_NO_THROW(t.validate());

    matrix = std::make_shared<OCIO::MatrixOpData>(OCIO::BIT_DEPTH_F16,
        OCIO::BIT_DEPTH_F32);
    t.getOps().push_back(matrix);

    OCIO_CHECK_THROW_WHAT(t.validate(), OCIO::Exception,
        "Bitdepth missmatch between ops");

    matrix->setInputBitDepth(OCIO::BIT_DEPTH_F32);
    OCIO_CHECK_NO_THROW(t.validate());
}
