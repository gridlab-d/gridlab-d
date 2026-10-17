"""Focused regression checks for fault-voltage property unit metadata.

Run from repository root: python3 -m unittest powerflow.autotest.test_fault_voltage_units -v
These targeted tests do not replace build or full model integration tests.
"""
import pathlib
import unittest

REPO = pathlib.Path(__file__).resolve().parents[2]

def read(path):
    return (REPO / path).read_text(encoding='utf-8')

class FaultVoltageUnitTests(unittest.TestCase):
    def test_fault_voltage_properties_use_volts(self):
        src = read('powerflow/link.cpp')
        self.assertIn('fault_voltage_A[V]', src)
        self.assertIn('fault_voltage_B[V]', src)
        self.assertIn('fault_voltage_C[V]', src)
