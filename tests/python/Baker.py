
import unittest, os, sys
import PyOpenColorIO as OCIO

class BakerTest(unittest.TestCase):
    
    def __init__(self, testName, sseBuild):
        super(BakerTest, self).__init__(testName)
        self.useSSE = sseBuild

    SIMPLE_PROFILE = """ocio_profile_version: 1

strictparsing: false

colorspaces:

  - !<ColorSpace>
    name: lnh
    bitdepth: 16f
    isdata: false
    allocation: lg2

  - !<ColorSpace>
    name: test
    bitdepth: 8ui
    isdata: false
    allocation: uniform
    to_reference: !<ExponentTransform> {value: [2.2, 2.2, 2.2, 1]}"""
    
    EXPECTED_LUT_NONSSE = """CSPLUTV100
3D

BEGIN METADATA
this is some metadata!
END METADATA

4
0.000977 0.039373 1.587401 64.000000
0.000000 0.333333 0.666667 1.000000
4
0.000977 0.039373 1.587401 64.000000
0.000000 0.333333 0.666667 1.000000
4
0.000977 0.039373 1.587401 64.000000
0.000000 0.333333 0.666667 1.000000

2 2 2
0.042823 0.042823 0.042823
6.622026 0.042823 0.042823
0.042823 6.622026 0.042823
6.622026 6.622026 0.042823
0.042823 0.042823 6.622026
6.622026 0.042823 6.622026
0.042823 6.622026 6.622026
6.622026 6.622026 6.622026

"""

    EXPECTED_LUT_SSE = """CSPLUTV100
3D

BEGIN METADATA
this is some metadata!
END METADATA

4
0.000977 0.039373 1.587398 64.000168
0.000000 0.333333 0.666667 1.000000
4
0.000977 0.039373 1.587398 64.000168
0.000000 0.333333 0.666667 1.000000
4
0.000977 0.039373 1.587398 64.000168
0.000000 0.333333 0.666667 1.000000

2 2 2
0.042823 0.042823 0.042823
6.622034 0.042823 0.042823
0.042823 6.622034 0.042823
6.622034 6.622034 0.042823
0.042823 0.042823 6.622034
6.622034 0.042823 6.622034
0.042823 6.622034 6.622034
6.622034 6.622034 6.622034

"""

    def test_interface(self):
        bake = OCIO.Baker()
        bakee = bake.createEditableCopy()
        cfg = OCIO.Config().CreateFromStream(self.SIMPLE_PROFILE)
        self.assertEqual(2, cfg.getNumColorSpaces())
        bakee.setConfig(cfg)
        cfg2 = bakee.getConfig()
        self.assertEqual(2, cfg2.getNumColorSpaces())
        bakee.setFormat("cinespace")
        self.assertEqual("cinespace", bakee.getFormat())
        bakee.setType("3D")
        self.assertEqual("3D", bakee.getType())
        bakee.setMetadata("this is some metadata!")
        self.assertEqual("this is some metadata!", bakee.getMetadata())
        bakee.setInputSpace("lnh")
        self.assertEqual("lnh", bakee.getInputSpace())
        bakee.setLooks("foo, +bar")
        self.assertEqual("foo, +bar", bakee.getLooks())
        bakee.setLooks("")
        bakee.setTargetSpace("test")
        self.assertEqual("test", bakee.getTargetSpace())
        bakee.setShaperSize(4)
        self.assertEqual(4, bakee.getShaperSize())
        bakee.setCubeSize(2)
        self.assertEqual(2, bakee.getCubeSize())
        output = bakee.bake()
        if self.useSSE == True:
            self.assertEqual(self.EXPECTED_LUT_SSE, output)
        else:
            self.assertEqual(self.EXPECTED_LUT_NONSSE, output)
        self.assertEqual(8, bakee.getNumFormats())
        self.assertEqual("cinespace", bakee.getFormatNameByIndex(2))
        self.assertEqual("3dl", bakee.getFormatExtensionByIndex(1))

