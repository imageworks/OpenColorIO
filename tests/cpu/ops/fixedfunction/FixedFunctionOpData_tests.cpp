// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "ops/fixedfunction/FixedFunctionOpData.cpp"

#include "UnitTest.h"

namespace OCIO = OCIO_NAMESPACE;

OCIO_ADD_TEST(FixedFunctionOpData, aces_red_mod_style)
{
    OCIO::FixedFunctionOpData func;
    OCIO_CHECK_EQUAL(func.getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_03_FWD);
    OCIO_CHECK_EQUAL(func.getParams().size(), 0);
    OCIO_CHECK_NO_THROW(func.validate());
    OCIO_CHECK_NO_THROW(func.finalize());
    const std::string cacheID(func.getCacheID());

    OCIO_CHECK_NO_THROW(func.setStyle(OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD));
    OCIO_CHECK_EQUAL(func.getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_10_FWD);
    OCIO_CHECK_NO_THROW(func.validate());
    OCIO_CHECK_NO_THROW(func.finalize());

    OCIO_CHECK_ASSERT(cacheID!=std::string(func.getCacheID()));

    OCIO::FixedFunctionOpDataRcPtr inv = func.inverse();
    OCIO_CHECK_EQUAL(inv->getStyle(), OCIO::FixedFunctionOpData::ACES_RED_MOD_10_INV);
    OCIO_CHECK_EQUAL(inv->getParams().size(), 0);
    OCIO_CHECK_ASSERT(cacheID!=std::string(inv->getCacheID()));

    OCIO::FixedFunctionOpData::Params p = func.getParams();
    p.push_back(1.);
    OCIO_CHECK_NO_THROW(func.setParams(p));
    OCIO_CHECK_THROW_WHAT(func.validate(), 
                          OCIO::Exception, 
                          "The style 'ACES_RedMod10 (Forward)' must have zero parameters but 1 found.");
}

OCIO_ADD_TEST(FixedFunctionOpData, aces_dark_to_dim10_style)
{
    OCIO::FixedFunctionOpData func(OCIO::FixedFunctionOpData::Params(), 
                                   OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD);

    OCIO_CHECK_EQUAL(func.getStyle(), OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_FWD);
    OCIO_CHECK_EQUAL(func.getParams().size(), 0);
    OCIO_CHECK_NO_THROW(func.validate());
    OCIO_CHECK_NO_THROW(func.finalize());
    const std::string cacheID(func.getCacheID());

    OCIO::FixedFunctionOpDataRcPtr inv = func.inverse();
    OCIO_CHECK_EQUAL(inv->getStyle(), OCIO::FixedFunctionOpData::ACES_DARK_TO_DIM_10_INV);
    OCIO_CHECK_EQUAL(inv->getParams().size(), 0);
    OCIO_CHECK_ASSERT(cacheID!=std::string(inv->getCacheID()));

    OCIO::FixedFunctionOpData::Params p = func.getParams();
    p.push_back(1.);
    OCIO_CHECK_NO_THROW(func.setParams(p));
    OCIO_CHECK_THROW_WHAT(func.validate(), 
                          OCIO::Exception, 
                          "The style 'ACES_DarkToDim10 (Forward)' must have zero parameters but 1 found.");
}

OCIO_ADD_TEST(FixedFunctionOpData, rec2100_surround_style)
{
    OCIO::FixedFunctionOpData::Params params = { 2.0 };
    OCIO::FixedFunctionOpData func(params, OCIO::FixedFunctionOpData::REC2100_SURROUND);
    OCIO_CHECK_NO_THROW(func.validate());
    OCIO_CHECK_NO_THROW(func.finalize());
    const std::string cacheID(func.getCacheID());
    OCIO_CHECK_ASSERT(func.getParams() == params);

    OCIO::FixedFunctionOpDataRcPtr inv = func.inverse();
    OCIO_CHECK_EQUAL(inv->getParams()[0], 1. / func.getParams()[0]);
    OCIO_CHECK_ASSERT(cacheID!=std::string(inv->getCacheID()));

    OCIO_CHECK_ASSERT(func == func);
    OCIO_CHECK_ASSERT(!(func == *inv));

    params = func.getParams();
    params[0] = 120.;
    OCIO_CHECK_NO_THROW(func.setParams(params));
    OCIO_CHECK_THROW_WHAT(func.validate(), 
                          OCIO::Exception,
                          "Parameter 120 is greater than upper bound 100");

    params = func.getParams();
    params[0] = 0.00001;
    OCIO_CHECK_NO_THROW(func.setParams(params));
    OCIO_CHECK_THROW_WHAT(func.validate(),
                          OCIO::Exception,
                          "Parameter 1e-05 is less than lower bound 0.001");

    params = func.getParams();
    params.push_back(12);
    OCIO_CHECK_NO_THROW(func.setParams(params));
    OCIO_CHECK_THROW_WHAT(func.validate(),
                          OCIO::Exception,
                          "The style 'REC2100_Surround' must have "
                          "one parameter but 2 found.");

    params = func.getParams();
    params.clear();
    OCIO_CHECK_NO_THROW(func.setParams(params));
    OCIO_CHECK_THROW_WHAT(func.validate(),
                          OCIO::Exception,
                          "The style 'REC2100_Surround' must have "
                          "one parameter but 0 found.");
}

