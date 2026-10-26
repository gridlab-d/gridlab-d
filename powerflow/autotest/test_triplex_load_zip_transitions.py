"""Execute the production update method with real GridLAB-D complex math.

The fixture supplies the object's storage and disables unrelated GFA/frequency
features. Full solver integration is still covered by the model validation job.
"""
import pathlib
import shutil
import subprocess
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[2]


class TriplexZIPTransitionTests(unittest.TestCase):
    def test_disable_manual_current_and_reenable_on_all_connections(self):
        compiler = shutil.which("c++")
        if not compiler:
            self.skipTest("C++ compiler unavailable")
        source = (ROOT / "powerflow/triplex_load.cpp").read_text()
        start = source.index("void triplex_load::triplex_load_update_fxn(void)")
        stop = source.index("// Function to appropriately zero load", start)
        method = source[start:stop]
        fixture = r'''
#include <cassert>
#include <cmath>
#include <complex>
#include "gld_complex.h"
struct OBJECT { const char *name = "fixture"; };
template<class T> OBJECT *object_header(T *) { static OBJECT o; return &o; }
void gl_warning(const char *, ...) {}
class triplex_load {
public:
  bool GFA_enable=false, GFA_status=true, enable_frequency_dependence=false;
  bool ZIP_constant_current[3]={}, base_load_val_was_nonzero[3]={};
  double base_power[3]={}, power_fraction[3]={}, current_fraction[3]={};
  double impedance_fraction[3]={};
  double power_pf[3]={1,1,1}, current_pf[3]={1,1,1}, impedance_pf[3]={1,1,1};
  double nominal_voltage=120, nominal_frequency=60, current_frequency=60;
  gld::complex voltage[3], voltage12, shunt[3], current[3], current12, power[3];
  gld::complex constant_power[3], constant_current[3], constant_impedance[3];
  gld::complex prev_load_values[3][3];
  gld::complex &voltage1=voltage[0], &voltage2=voltage[1];
  void triplex_load_delete_update_fxn() {}
  void triplex_load_update_fxn();
};
'''
        assertions = r'''
void close(gld::complex actual, std::complex<double> expected) {
  assert(std::abs(actual.Re()-expected.real()) < 1e-10);
  assert(std::abs(actual.Im()-expected.imag()) < 1e-10);
}
int main() {
  for (int phase=0; phase<3; ++phase) {
    for (bool disable_fraction : {false, true}) {
      triplex_load x;
      x.voltage[0].SetPolar(120,0.3);
      x.voltage[1].SetPolar(120,-0.2);
      auto v = phase==2 ? x.voltage[0]+x.voltage[1] : x.voltage[phase];
      const double nominal = phase==2 ? 240 : 120;
      auto accumulated = [&]() {return phase==2 ? x.current12 : x.current[phase];};
      x.base_power[phase]=1200;
      x.current_fraction[phase]=1;
      x.triplex_load_update_fxn();
      assert(x.ZIP_constant_current[phase]);
      close(accumulated(), std::polar(1200/nominal,v.Arg()));
      // Repeated updates must replace the previous contribution, not add it.
      x.triplex_load_update_fxn();
      close(accumulated(), std::polar(1200/nominal,v.Arg()));
      if (disable_fraction) {
        x.current_fraction[phase]=0;
        x.power_fraction[phase]=1;
      } else {
        x.base_power[phase]=0;
      }
      x.triplex_load_update_fxn();
      assert(!x.ZIP_constant_current[phase]);
      close(x.constant_current[phase], {0,0});
      close(accumulated(), {0,0});
      x.base_power[phase]=0;
      x.triplex_load_update_fxn();
      // With ZIP disabled, preserve the supplied current and apply the
      // manual-current voltage-angle adjustment exactly once.
      x.constant_current[phase]=gld::complex(3,4);
      x.triplex_load_update_fxn();
      close(x.constant_current[phase], {3,4});
      close(accumulated(), std::complex<double>(3,4)*std::polar(1.0,v.Arg()));
      x.base_power[phase]=1200;
      x.current_fraction[phase]=1;
      x.power_fraction[phase]=0;
      x.triplex_load_update_fxn();
      assert(x.ZIP_constant_current[phase]);
      close(accumulated(), std::polar(1200/nominal,v.Arg()));
    }
  }
}
'''
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder)
            # The complex header only needs platform definitions; the fixture
            # does not require a generated simulator build configuration.
            (path / "config.h").write_text("")
            (path / "case.cpp").write_text(fixture + method + assertions)
            compiled = subprocess.run(
                [compiler, "-std=c++20", "-DHAVE_CONFIG_H", "-Wall", "-Wextra",
                 "-I", str(ROOT / "gldcore"), "-I", str(path),
                 str(path / "case.cpp"), "-o", str(path / "case")],
                capture_output=True, text=True, timeout=60,
            )
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            subprocess.run([str(path / "case")], check=True, timeout=10)
