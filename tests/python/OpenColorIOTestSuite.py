# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import logging
import unittest
import os
import sys

logging.basicConfig(
    level=logging.INFO,
    format="[%(levelname)s] %(name)s: %(message)s",
)

build_location = sys.argv[1]
os.environ["BUILD_LOCATION"] = build_location

opencolorio_sse = sys.argv[2].lower() == 'true'

opencolorio_dir = os.path.join(build_location, 'lib')
pyopencolorio_dir = os.path.join(build_location, 'lib', 'python3.8', 'site-packages')

if os.name == 'nt':
    # On Windows we must append the build type to the build dirs and add the main library to PATH
    # Note: Only when compiling within Microsoft Visual Studio editor i.e. not on command line.
    if len(sys.argv) == 4:
        opencolorio_dir = os.path.join(opencolorio_dir, sys.argv[3])
        pyopencolorio_dir = os.path.join(pyopencolorio_dir, sys.argv[3])

    # Python 3.8+ does no longer look for DLLs in PATH environment variable
    if hasattr(os, 'add_dll_directory'):
        os.add_dll_directory(opencolorio_dir)
    else:
        os.environ['PATH'] = '{0};{1}'.format(
            opencolorio_dir, os.getenv('PATH', ''))
elif sys.platform == 'darwin':
    # On OSX we must add the main library location to DYLD_LIBRARY_PATH
    os.environ['DYLD_LIBRARY_PATH'] = '{0}:{1}'.format(
        opencolorio_dir, os.getenv('DYLD_LIBRARY_PATH', ''))

sys.path.insert(0, pyopencolorio_dir)
import PyOpenColorIO as OCIO

import AllocationTransformTest
import BuiltinTransformRegistryTest
import BuiltinTransformTest
import CDLTransformTest
import CPUProcessorTest
import ColorSpaceHelpersTest
import ColorSpaceTest
import ColorSpaceTransformTest
import ConfigTest
import DisplayViewHelpersTest
import DisplayViewTransformTest
import ExponentTransformTest
import ExponentWithLinearTransformTest
import ExposureContrastTransformTest
import FileTransformTest
import FixedFunctionTransformTest
import FormatMetadataTest
import GradingDataTest
import GradingPrimaryTransformTest
import GradingRGBCurveTransformTest
import GradingToneTransformTest
import GroupTransformTest
import LegacyViewingPipelineTest
import LogTransformTest
import LookTest
import MixingHelpersTest
import NamedTransformTest
import OpenColorIOTest
import TransformInterfaceTest
import ViewingRulesTest
#from MainTest import *
#from ConstantsTest import *
#from ConfigTest import *
#from ContextTest import *
#from GpuShaderDescTest import *
#from Baker import *
#from TransformsTest import *
#from RangeTransformTest import *

def suite():
    """Load unittest.TestCase objects from *Test.py files within ./tests/Python

    :return: unittest test suite of TestCase objects.
    :rtype: unittest.TestSuite
    """

    # top level directory cached on loader instance
    suite = unittest.TestSuite()
    loader = unittest.TestLoader()

    suite.addTest(loader.loadTestsFromModule(AllocationTransformTest))
    suite.addTest(loader.loadTestsFromModule(BuiltinTransformRegistryTest))
    suite.addTest(loader.loadTestsFromModule(BuiltinTransformTest))
    suite.addTest(loader.loadTestsFromModule(CDLTransformTest))
    suite.addTest(loader.loadTestsFromModule(CPUProcessorTest))
    suite.addTest(loader.loadTestsFromModule(ColorSpaceHelpersTest))
    suite.addTest(loader.loadTestsFromModule(ColorSpaceTest))
    suite.addTest(loader.loadTestsFromModule(ColorSpaceTransformTest))
    suite.addTest(loader.loadTestsFromModule(ConfigTest))
    suite.addTest(loader.loadTestsFromModule(DisplayViewHelpersTest))
    suite.addTest(loader.loadTestsFromModule(DisplayViewTransformTest))
    suite.addTest(loader.loadTestsFromModule(ExponentTransformTest))
    suite.addTest(loader.loadTestsFromModule(ExponentWithLinearTransformTest))
    suite.addTest(loader.loadTestsFromModule(ExposureContrastTransformTest))
    suite.addTest(loader.loadTestsFromModule(FileTransformTest))
    suite.addTest(loader.loadTestsFromModule(FixedFunctionTransformTest))
    suite.addTest(loader.loadTestsFromModule(FormatMetadataTest))
    suite.addTest(loader.loadTestsFromModule(GradingDataTest))
    suite.addTest(loader.loadTestsFromModule(GradingPrimaryTransformTest))
    suite.addTest(loader.loadTestsFromModule(GradingRGBCurveTransformTest))
    suite.addTest(loader.loadTestsFromModule(GradingToneTransformTest))
    suite.addTest(loader.loadTestsFromModule(GroupTransformTest))
    suite.addTest(loader.loadTestsFromModule(LegacyViewingPipelineTest))
    suite.addTest(loader.loadTestsFromModule(LogTransformTest))
    suite.addTest(loader.loadTestsFromModule(LookTest))
    suite.addTest(loader.loadTestsFromModule(MixingHelpersTest))
    suite.addTest(loader.loadTestsFromModule(NamedTransformTest))
    suite.addTest(loader.loadTestsFromModule(OpenColorIOTest))
    suite.addTest(loader.loadTestsFromModule(TransformInterfaceTest))
    suite.addTest(loader.loadTestsFromModule(ViewingRulesTest))
    #suite.addTest(MainTest("test_interface"))
    #suite.addTest(ConstantsTest("test_interface"))
    #suite.addTest(ConfigTest("test_interface"))
    #suite.addTest(ConfigTest("test_is_editable"))
    #suite.addTest(ContextTest("test_interface"))
    #suite.addTest(RangeTransformTest("test_interface"))
    #suite.addTest(RangeTransformTest("test_equality"))
    #suite.addTest(RangeTransformTest("test_validation"))
    #suite.addTest(TransformsTest("test_interface"))

    # Processor
    # ProcessorMetadata
    #suite.addTest(GpuShaderDescTest("test_interface"))
    #suite.addTest(BakerTest("test_interface", opencolorio_sse))
    # PackedImageDesc
    # PlanarImageDesc

    return suite


if __name__ == '__main__':
    runner = unittest.TextTestRunner(verbosity=2)
    test_suite = suite()
    result = runner.run(test_suite)
    if result.wasSuccessful() == False:
        sys.exit(1)
    sys.exit(0)
