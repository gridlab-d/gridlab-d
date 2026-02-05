from argparse import ArgumentParser
from pathlib import Path
import math
import multiprocessing
import shutil
import subprocess
import sys
import time

autotestFiles = []

def processModuleDirectory(moduleDirectory: Path, runOptionalTests: bool):
    """
    Search through a module directory for a directory called autotest. Copy autotest to it's own individual directory 
    to be run from. Capture all autotest files present in the autotest directory.
    """
    global autotestFiles
    noValidateFile = moduleDirectory / "validate.no"
    noValidateFile.resolve()
    if not noValidateFile.exists():    
        autotestDirectory = moduleDirectory / "autotest"
        autotestDirectory.resolve()
        if autotestDirectory.exists() and autotestDirectory.is_dir():
            for autotestChild in autotestDirectory.iterdir():
                autotestChild.resolve()
                if (autotestChild.is_file() 
                        and (autotestChild.suffix == ".glm" or autotestChild.suffix == ".json") 
                        and "test_" in autotestChild.stem
                        and (("_opt" in autotestChild.stem and runOptionalTests) or "_opt" not in autotestChild.stem)):
                    autotestFiles.append(autotestChild)
                    autotestDir = autotestChild.parent / autotestChild.stem
                    autotestDir.resolve()
                    if not autotestDir.exists():
                        autotestDir.mkdir()
                    shutil.copy(autotestChild, autotestDir)


def runAutotest(autotestFile: Path) -> int:
    """
    Run a single autotest file using GridLAB-D.
    """
    print(f"Running autotest: {autotestFile}")
    autotestDir = autotestFile.parent / autotestFile.stem
    autotestDir.resolve()
    command = ["gridlabd", str(autotestFile.name)]
    result = subprocess.run(
        command,
        cwd=autotestDir,
        capture_output=True
    )
    rv = 0
    if result.returncode != 0 and "_err" not in autotestFile.stem:
        rv = 1
    elif result.returncode == 0 and "_err" in autotestFile.stem:
        rv = 2
    return rv


def getGLDVersionInfo() -> str:
    """
    Get the GridLAB-D version information.
    """
    command = ["gridlabd", "--version"]
    result = subprocess.run(
        command,
        capture_output=True,
        text=True
    )
    if result.returncode == 0:
        return result.stdout.strip()
    return "Unknown GridLAB-D version"


def processResults(results: list[int], resultsFile: Path, testPerformance: int) -> int:
    """
    Process the results of the autotest runs and output to validate.txt.
    """
    global autotestFiles
    rv = 0
    gldInfo = getGLDVersionInfo()
    with resultsFile.open("w") as f:
        f.write(f"GridLAB-D Version Info:\n\t{gldInfo}\n\n")
        f.write("Autotest Results:\n")
        passCount = 0
        failCount = 0
        unexpectedPassCount = 0
        for i, result in enumerate(results):
            autotestFile = autotestFiles[i]
            if result == 0:
                passCount += 1
                f.write(f"\t[PASS]\t{autotestFile}\n")
            elif result == 1:
                failCount += 1
                f.write(f"\t[FAIL]\t{autotestFile} - Expected to pass but failed.\n")
            elif result == 2:
                unexpectedPassCount += 1
                f.write(f"\t[FAIL]\t{autotestFile} - Expected to fail but passed.\n")
        f.write("\nResults Summary:\n")
        f.write(f"\tTotal Tests Run: {len(results)}.\n")
        f.write(f"\tTotal Tests Pass: {passCount}.\n")
        f.write(f"\tTotal Tests Fail: {failCount}.\n")
        f.write(f"\tTotal Unexpected Passes: {unexpectedPassCount}.\n")
        f.write(f"\tPass Percentage: {math.floor((float(passCount) / float(len(results))) * 100.0)}%.\n")
        f.write(f"\tTotal Test Time: {testPerformance} seconds.")
        print("\nResults Summary:\n"
              f"Total Tests Run: {len(results)}.\n"
              f"Total Tests Pass: {passCount}.\n"
              f"Total Tests Fail: {failCount}.\n"
              f"Total Unexpected Passes: {unexpectedPassCount}.\n"
              f"Pass Percentage: {math.floor((float(passCount) / float(len(results))) * 100.0)}%.\n"
              f"Total Test Time: {testPerformance} seconds.\n"
              f"Full results written to {resultsFile}.")
    if failCount > 0 or unexpectedPassCount > 0:
        rv = 1
    return rv

def main(module: str, runOptionalTests: bool, threads: int):
    global autotestFiles
    rv = 0
    multiprocessing.set_start_method("spawn", force=True)
    procs = 1
    if threads < 1:
        procs = multiprocessing.cpu_count()
    else:
        procs = min(threads, multiprocessing.cpu_count())
    searchDirectoryBase = Path(__file__).parent.resolve()
    moduleDirectory = None
    resultsFile = searchDirectoryBase / "validate.txt"
    resultsFile.resolve()
    if resultsFile.exists():
        resultsFile.unlink()
    # Search for autotest files
    if module != "all":
        moduleDirectory = searchDirectoryBase / module
        moduleDirectory.resolve()
        if not moduleDirectory.exists() or not moduleDirectory.is_dir():
            raise IOError(f"The module, {module}, does not exist.")
        processModuleDirectory(moduleDirectory, runOptionalTests)
        resultsFile = moduleDirectory / "validate.txt"
        resultsFile.resolve()
        if resultsFile.exists():
            resultsFile.unlink()
    else:
        for moduleChild in searchDirectoryBase.iterdir():
            if moduleChild.is_dir():
                processModuleDirectory(moduleChild.resolve(), runOptionalTests)
    # Run autotests in parallel
    autotestFiles.sort()
    if len(autotestFiles) > 0:
        procs = min(procs, len(autotestFiles))
        results = []
        startTime = time.perf_counter_ns()
        with multiprocessing.Pool(procs) as p:
            results = p.map(runAutotest, autotestFiles)
        endTime = time.perf_counter_ns()
        testPerformance = math.ceil((endTime - startTime) / 1.0e9)
        rv = processResults(results, resultsFile, testPerformance)
    elif module != "all":
            print(f"No autotest files were found recursively searching from {moduleDirectory}. Exiting.")
    else:
            print(f"No autotest files were found recursively searching from {searchDirectoryBase}.Exiting.")
    if rv == 1:
        sys.exit(1)


if __name__ == "__main__":
    parser = ArgumentParser(description="Validate GridLAB-D by running the autotests.")
    parser.add_argument(
        "-m",
        "--module",
        type = str,
        default = "all",
        help = "The Specific module to validate. Default is all modules."
    )
    parser.add_argument(
        "-o",
        "--run_optional_tests",
        action="store_true",
        help = "Run optional tests as well."
    )
    parser.add_argument(
        "-T",
        "--threads",
        type = int,
        default = 1,
        help = "Number of threads to use when running tests in parallel. Default is 1."
    )
    args = parser.parse_args()
    main(args.module, args.run_optional_tests, args.threads)
