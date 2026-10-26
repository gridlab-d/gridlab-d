"""Run focused Python regressions that model validation does not discover."""
from pathlib import Path
import shutil
import unittest


def main():
    if shutil.which("c++") is None:
        raise SystemExit("A C++ compiler is required for the compiled regressions")
    root = Path(__file__).resolve().parents[1]
    suite = unittest.TestSuite()
    for module in ("gldcore", "powerflow", "connection"):
        directory = root / module / "autotest"
        if any(directory.glob("test_*.py")):
            # Separate loaders support these non-package test directories.
            suite.addTests(unittest.TestLoader().discover(str(directory), "test_*.py"))
    print(f"Discovered {suite.countTestCases()} focused Python regressions", flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    raise SystemExit(0 if result.wasSuccessful() else 1)


if __name__ == "__main__":
    main()
