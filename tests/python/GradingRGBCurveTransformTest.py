# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import assertEqualRGBCurve

class GradingRGBCurveTransformTest(unittest.TestCase):

    valDefaultLin = OCIO.GradingRGBCurve(OCIO.GRADING_LIN)
    valDefaultLog = OCIO.GradingRGBCurve(OCIO.GRADING_LOG)

    def test_transform_type(self):
        """
        Test the getTransformType() method.
        """
        gct = OCIO.GradingRGBCurveTransform()
        self.assertEqual(gct.getTransformType(), OCIO.TRANSFORM_TYPE_GRADING_RGB_CURVE)

    def test_contructor(self):
        """
        Test GradingRGBCurveTransform constructor without and with keywords.
        """

        gct = OCIO.GradingRGBCurveTransform()
        self.assertEqual(gct.getStyle(), OCIO.GRADING_LOG)
        assertEqualRGBCurve(self, gct.getValue(), self.valDefaultLog)
        self.assertEqual(gct.isDynamic(), False)
        self.assertEqual(gct.getBypassLinToLog(), False)
        self.assertEqual(gct.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        gct = OCIO.GradingRGBCurveTransform(OCIO.GRADING_LIN)
        self.assertEqual(gct.getStyle(), OCIO.GRADING_LIN)
        assertEqualRGBCurve(self, gct.getValue(), self.valDefaultLin)
        self.assertEqual(gct.isDynamic(), False)
        self.assertEqual(gct.getBypassLinToLog(), False)
        self.assertEqual(gct.getDirection(), OCIO.TRANSFORM_DIR_FORWARD)

        vals = OCIO.GradingRGBCurve(OCIO.GRADING_LOG)
        vals.red = OCIO.GradingBSplineCurve(4)
        cpts = vals.red.getControlPoints()
        cpts[0] = OCIO.GradingControlPoint(0.0, 0.1)
        cpts[1] = OCIO.GradingControlPoint(0.1, 0.5)
        cpts[2] = OCIO.GradingControlPoint(0.4, 0.6)
        cpts[3] = OCIO.GradingControlPoint(0.6, 0.7)
        gct = OCIO.GradingRGBCurveTransform(style=OCIO.GRADING_VIDEO, values=vals,
                                            dynamic=True, dir=OCIO.TRANSFORM_DIR_INVERSE)
        self.assertEqual(gct.getStyle(), OCIO.GRADING_VIDEO)
        self.assertEqual(gct.isDynamic(), True)
        self.assertEqual(gct.getDirection(), OCIO.TRANSFORM_DIR_INVERSE)
        assertEqualRGBCurve(self, gct.getValue(), vals)

    def test_style(self):
        """
        Test setStyle() and getStyle().
        """

        gct = OCIO.GradingRGBCurveTransform(OCIO.GRADING_LOG)
        for style in OCIO.GradingStyle.__members__.values():
            gct.setStyle(style)
            self.assertEqual(gct.getStyle(), style)

    def test_values(self):
        """
        Test setValue() and getValue().
        """

        gct = OCIO.GradingRGBCurveTransform(OCIO.GRADING_LOG)
        gct.setValue(self.valDefaultLin)
        assertEqualRGBCurve(self, gct.getValue(), self.valDefaultLin)

    def test_dynamic(self):
        """
        Test isDynamic() and makeDynamic().
        """

        gct = OCIO.GradingRGBCurveTransform(OCIO.GRADING_LOG)
        self.assertEqual(gct.isDynamic(), False)
        gct.makeDynamic()
        self.assertEqual(gct.isDynamic(), True)

    def test_validation(self):
        """
        Test validate() and setValue().
        """

        gct = OCIO.GradingRGBCurveTransform(OCIO.GRADING_LOG)
        gct.validate()

        # 3rd control point x is lower than 2nd control point x.
        vals = OCIO.GradingRGBCurve(OCIO.GRADING_LOG)
        vals.red = OCIO.GradingBSplineCurve([0, 0, 0.5, 0.2, 0.2, 0.5, 1, 1])
        
        with self.assertRaises(OCIO.Exception):
            gct.setValue(vals);

    def test_apply_inverse(self):
        """
        Test applying transform with inversion.
        """

        gct = OCIO.GradingRGBCurveTransform(OCIO.GRADING_LOG)
        vals = OCIO.GradingRGBCurve(OCIO.GRADING_LOG)
        vals.red = OCIO.GradingBSplineCurve([0, 0, 0.4, 0.2, 0.5, 0.7, 0.6, 1.5, 1, 2.1])
        vals.green = OCIO.GradingBSplineCurve([-0.1, -0.5, 0.5, 0.2, 1.5, 1.1])
        vals.blue = OCIO.GradingBSplineCurve([0, 0, 1, 1])
        vals.master = OCIO.GradingBSplineCurve([0, -0.2, 0.3, 0.7, 1.2, 1.5, 2.5, 2.2])
        gct.setValue(vals)

        cfg = OCIO.Config().CreateRaw()
        proc = cfg.getProcessor(gct)
        cpu = proc.getDefaultCPUProcessor()

        # Apply the transform and keep the result.
        pixel = [0.48, 0.18, 0.18]
        rgb1 = cpu.applyRGB(pixel)

        # The processing did something.
        self.assertAlmostEqual( 1.010323, rgb1[0], delta=1e-5)
        self.assertAlmostEqual(-0.770639, rgb1[1], delta=1e-5)
        self.assertAlmostEqual( 0.398450, rgb1[2], delta=1e-5)

        # Invert.
        gct.setDirection(OCIO.TRANSFORM_DIR_INVERSE)
        proc = cfg.getProcessor(gct)
        cpu = proc.getDefaultCPUProcessor()
        pixel2 = cpu.applyRGB(rgb1)

        # Invert back to original value.
        self.assertAlmostEqual(pixel[0], pixel2[0], delta=1e-5)
        self.assertAlmostEqual(pixel[1], pixel2[1], delta=1e-5)
        self.assertAlmostEqual(pixel[2], pixel2[2], delta=1e-5)
