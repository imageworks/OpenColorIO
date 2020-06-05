# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

import unittest
import os
import sys

import PyOpenColorIO as OCIO
from UnitTestUtils import TEST_NAMES, TEST_DESCS


class LookTest(unittest.TestCase):
    TEST_PROCESS_SPACES = ['raw', 'lnh', 'vd8', 'a.b.c.', '1-2-3-']

    def setUp(self):
        self.look = OCIO.Look()

    def tearDown(self):
        self.look = None

    def test_name(self):
        """
        Test the setName() and getName() methods.
        """

        # Default initialized name value is ""
        self.assertEqual(self.look.getName(), '')

        for name in TEST_NAMES:
            self.look.setName(name)
            self.assertEqual(name, self.look.getName())

        # Wrong type tests.
        with self.assertRaises(TypeError):
            self.look.setName(None)
            self.look.setName(1)

    def test_process(self):
        """
        Test the setProcessSpace() and getProcessName() methods.
        """

        # Default initialized process space value is ""
        self.assertEqual(self.look.getProcessSpace(), '')

        for process_space in self.TEST_PROCESS_SPACES:
            self.look.setProcessSpace(process_space)
            self.assertEqual(process_space, self.look.getProcessSpace())

        # Wrong type tests.
        with self.assertRaises(TypeError):
            self.look.setProcessSpace(None)
            self.look.setProcessSpace(1)

    def test_description(self):
        """
        Test the setDescription() and getDescription() methods.
        """

        # Default initialized description value is ""
        self.assertEqual(self.look.getDescription(), '')

        for desc in TEST_DESCS:
            self.look.setDescription(desc)
            self.assertEqual(desc, self.look.getDescription())

        # Wrong type tests.
        with self.assertRaises(TypeError):
            self.look.setDescription(None)
            self.look.setDescription(1)

    def test_transform(self):
        """
        Test the setTransform() and getTransform() methods.
        """

        # Default initialized transform value is None
        self.assertIsNone(self.look.getTransform())

        exp_tr = OCIO.ExponentTransform()
        exp_values = [0.1, 0.2, 0.3, 0.4]
        exp_tr.setValue(exp_values)
        self.look.setTransform(exp_tr)
        oet = self.look.getTransform()
        self.assertListEqual(oet.getValue(), exp_values)

        # Wrong type tests.
        with self.assertRaises(TypeError):
            self.look.setTransform(OCIO.ALLOCATION_UNIFORM)
            self.look.setTransform(1)

    def test_inverse_transform(self):
        """
        Test the setInverseTransform() and getInverseTransform() methods.
        """

        # Default initialized inverse transform value is None
        self.assertIsNone(self.look.getInverseTransform())

        exp_tr = OCIO.ExponentTransform()
        exp_values = [-0.1, -0.2, -0.3, -0.4]
        exp_tr.setValue(exp_values)
        self.look.setInverseTransform(exp_tr)
        inv_oet = self.look.getInverseTransform()
        self.assertListEqual(inv_oet.getValue(), exp_values)

        # Wrong type tests.
        with self.assertRaises(TypeError):
            self.look.setInverseTransform(OCIO.ALLOCATION_UNIFORM)
            self.look.setInverseTransform(1)

    def test_constructor_with_keyword(self):
        """
        Test Look constructor with keywords and validate its values.
        """

        # With keywords in their proper order.
        exp_tr = OCIO.ExponentTransform()
        inv_exp_tr = OCIO.ExponentTransform()
        look = OCIO.Look(name='coollook',
                         processSpace='somespace',
                         transform=exp_tr,
                         inverseTransform=inv_exp_tr,
                         description='this is a test')

        self.assertEqual(look.getName(), 'coollook')
        self.assertEqual(look.getProcessSpace(), 'somespace')
        self.assertIsInstance(look.getTransform(), type(exp_tr))
        self.assertIsInstance(look.getInverseTransform(), type(inv_exp_tr))
        self.assertEqual(look.getDescription(), 'this is a test')

        # With keyword not in their proper order.
        exp_tr2 = OCIO.ExponentTransform()
        inv_exp_tr2 = OCIO.ExponentTransform()
        look2 = OCIO.Look(inverseTransform=inv_exp_tr,
                          description='this is a test',
                          name='coollook',
                          processSpace='somespace',
                          transform=exp_tr)

        self.assertEqual(look2.getName(), 'coollook')
        self.assertEqual(look2.getProcessSpace(), 'somespace')
        self.assertIsInstance(look2.getTransform(), type(exp_tr2))
        self.assertIsInstance(look2.getInverseTransform(), type(inv_exp_tr2))
        self.assertEqual(look2.getDescription(), 'this is a test')

    def test_constructor_without_keyword(self):
        """
        Test Look constructor without keywords and validate its values.
        """

        exp_tr = OCIO.ExponentTransform()
        inv_exp_tr = OCIO.ExponentTransform()
        look = OCIO.Look('coollook',
                         'somespace',
                         exp_tr,
                         inv_exp_tr,
                         'this is a test')

        self.assertEqual(look.getName(), 'coollook')
        self.assertEqual(look.getProcessSpace(), 'somespace')
        self.assertIsInstance(look.getTransform(), type(exp_tr))
        self.assertIsInstance(look.getInverseTransform(), type(inv_exp_tr))
        self.assertEqual(look.getDescription(), 'this is a test')

    def test_constructor_without_parameter(self):
        """
        Test Look default constructor and validate its values.
        """

        look = OCIO.Look()

        self.assertEqual(look.getName(), '')
        self.assertEqual(look.getProcessSpace(), '')
        self.assertIsNone(look.getTransform())
        self.assertIsNone(look.getInverseTransform())
        self.assertEqual(look.getDescription(), '')

    def test_constructor_wrong_parameter_type(self):
        """
        Test Look constructor with a wrong parameter type.
        """

        with self.assertRaises(TypeError):
            look = OCIO.Look(None)
            look = OCIO.Look(1)
