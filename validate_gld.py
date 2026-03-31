from argparse import ArgumentParser
from pathlib import Path
import math
import multiprocessing
import shutil
import subprocess
import sys
import time

autotestFiles = []
gldBinary = None


def getGLDBinary():
    """
    Check developement environment for the gridlabd binary file.
    """
    global gldBinary
    gldBinary = shutil.which("gridlabd")
    if not gldBinary:
        rootPath = Path(__file__).parent.resolve()
        for child in rootPath.iterdir():
            if child.is_dir():
                childBin = child / "bin" / "gridlabd"
                childBin.resolve()
                if childBin.exists() and childBin.is_file():
                    gldBinary = f"{childBin}"
                    break
    if not gldBinary:
        raise ModuleNotFoundError(
            "Could not find the gridlabd binary in the development environment!"
        )


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
                if (
                    autotestChild.is_file()
                    and (
                        autotestChild.suffix == ".glm"
                        or autotestChild.suffix == ".json"
                    )
                    and "test_" in autotestChild.stem
                    and (
                        ("_opt" in autotestChild.stem and runOptionalTests)
                        or "_opt" not in autotestChild.stem
                    )
                ):
                    autotestFiles.append((autotestChild, gldBinary))
                    autotestDir = autotestChild.parent / autotestChild.stem
                    autotestDir.resolve()
                    if not autotestDir.exists():
                        autotestDir.mkdir()
                    shutil.copy(autotestChild, autotestDir)



def runAutotest(args: tuple[Path, str]) -> tuple[int, Path]:
    """
    Run a single autotest file using GridLAB-D.
    """
    autotestFile, binFile = args

    # Parent directory that holds the model and all its assets (players, CSVs, etc.)
    src_dir = autotestFile.parent                 # e.g., .../<module>/autotest

    # Per-test work directory (where logs like gridlabd.err/out will be written)
    work_dir = src_dir / autotestFile.stem        # e.g., .../autotest/test_foo
    work_dir.resolve()
    if not work_dir.exists():
        work_dir.mkdir()

    # Compose command: run from parent directory, write outputs into work_dir
    command = [binFile, autotestFile.name]
    (work_dir / "gridlabd.start").write_text(f"RUN {autotestFile.name} via {binFile}\n")

                                             
    env = dict(os.environ)
    per_test_timeout_s = int(env.get("GLD_TEST_TIMEOUT", os.environ.get("GLD_TIMEOUT", "600")))


    # # Capture raw bytes to avoid UnicodeDecodeError
    # result = subprocess.run(
    #     command,
    #     cwd=work_dir,
    #     stdout=subprocess.PIPE,
    #     stderr=subprocess.PIPE,
    #     text=False,  # <-- CRITICAL: captures bytes, no decoding
    # )

    print(f"[run] {autotestFile}", flush=True)
    try:
            result = subprocess.run(
                command,
                cwd=work_dir,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=False,  # capture bytes
                env=env,
                timeout=per_test_timeout_s,
            )
    except subprocess.TimeoutExpired as e:
            # Persist a clear timeout marker
            (work_dir / "gridlabd.out").write_text("")
            (work_dir / "gridlabd.err").write_text(f"TIMEOUT after {per_test_timeout_s}s\n{e}")
            return (1, work_dir / autotestFile.name)


    # Persist outputs (parity with validate.cpp)
    (work_dir / "gridlabd.out").write_bytes(result.stdout)
    (work_dir / "gridlabd.err").write_bytes(result.stderr)

    # # Classification logic (same as your harness)
    # rv = 0
    # if result.returncode != 0 and "_err" not in autotestFile.stem:
    #     rv = 1
    # elif result.returncode == 0 and "_err" in autotestFile.stem:
    #     rv = 2
    # return (rv, work_dir / autotestFile.name)


    
    rv = 0
    # Signal termination: negative return code on POSIX
    if result.returncode < 0:
        rv = 1
        # Append a signal note
        try:
            existing = (work_dir / "gridlabd.err").read_text()
        except Exception:
            existing = ""
        (work_dir / "gridlabd.err").write_text(existing + f"\nPROCESS TERMINATED BY SIGNAL {abs(result.returncode)}")
    elif result.returncode != 0 and "_err" not in autotestFile.stem:
        rv = 1
    elif result.returncode == 0 and "_err" in autotestFile.stem:
        rv = 2
    return (rv, work_dir / autotestFile.name)


def getGLDVersionInfo() -> str:
    """
    Get the GridLAB-D version information.
    """
    command = [gldBinary, "--version"]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode == 0:
        return result.stdout.strip()
    return "Unknown GridLAB-D version"


def processResults(results: list[tuple], resultsFile: Path, testPerformance: int) -> int:
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
        passingTests = []
        failingTests1 = []
        failingTests2 = []
        for resultTuple in results:
            result, autotestFile = resultTuple
            if result == 0:
                passCount += 1
                passingTests.append(autotestFile)
            elif result == 1:
                failCount += 1
                failingTests1.append(autotestFile)
            elif result == 2:
                unexpectedPassCount += 1
                failingTests2.append(autotestFile)
        if passCount > 0:
            f.write(f"\tPassing Tests:")
            for test in passingTests:
                f.write(f"\n\t\t[PASS]\t{test}")
        if failCount > 0 or unexpectedPassCount > 0:
            f.write(f"\n\tFailing Tests:")
            for test in failingTests1:
                f.write(f"\n\t\t[FAIL]\t{test}...failed to run but was expected to pass.")
            for test in failingTests2:
                f.write(f"\n\t\t[FAIL]\t{test}...ran successfully but was expected to fail.")
        f.write("\nResults Summary:\n")
        f.write(f"\tTotal Tests Run: {len(results)}.\n")
        f.write(f"\tTotal Tests Pass: {passCount}.\n")
        f.write(f"\tTotal Tests Fail: {failCount + unexpectedPassCount}.\n")
        f.write(
            f"\tPass Percentage: {math.floor((float(passCount) / float(len(results))) * 100.0)}%.\n"
        )
        f.write(f"\tTotal Test Time: {testPerformance} seconds.")
        print(
            "\nResults Summary:\n"
            f"Total Tests Run: {len(results)}.\n"
            f"Total Tests Pass: {passCount}.\n"
            f"Total Tests Fail: {failCount + unexpectedPassCount}.\n"
            f"Pass Percentage: {math.floor((float(passCount) / float(len(results))) * 100.0)}%.\n"
            f"Total Test Time: {testPerformance} seconds.\n"
            f"Full results written to {resultsFile}."
        )
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
    getGLDBinary()
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
    autotestFiles.sort(key=lambda x: x[0])
    if len(autotestFiles) > 0:
        procs = min(procs, len(autotestFiles))
        results = []
        startTime = time.perf_counter_ns()
        done = 0
        total = len(autotestFiles)
        if procs > 1:
            with multiprocessing.Pool(procs) as p:
                # results = p.starmap(runAutotest, autotestFiles)
                for rv in p.imap_unordered(runAutotest, autotestFiles):  # <-- no lambda here
                    results.append(rv)
                    done += 1
                    percentDone = math.floor((float(done) / float(total)) * 100.0)
                    # Emit progress every few completions (and at the end)
                    print(f"[progress] {percentDone}% autotests completed...", flush=True, end="\r")
        else:
            for tup in autotestFiles:
                results.append(runAutotest(tup))
                done += 1
                percentDone = math.floor((float(done) / float(total)) * 100.0)
                # Emit progress every few completions (and at the end)
                print(f"[progress] {percentDone}% autotests completed...", flush=True, end="\r")
        endTime = time.perf_counter_ns()
        testPerformance = math.ceil((endTime - startTime) / 1.0e9)
        rv = processResults(results, resultsFile, testPerformance)
    elif module != "all":
        print(
            f"No autotest files were found recursively searching from {moduleDirectory}. Exiting."
        )
    else:
        print(
            f"No autotest files were found recursively searching from {searchDirectoryBase}.Exiting."
        )
    if rv == 1:
        sys.exit(1)


if __name__ == "__main__":
    parser = ArgumentParser(description="Validate GridLAB-D by running the autotests.")
    parser.add_argument(
        "-m",
        "--module",
        type=str,
        default="all",
        help="The Specific module to validate. Default is all modules.",
    )
    parser.add_argument(
        "-o",
        "--run_optional_tests",
        action="store_true",
        help="Run optional tests as well.",
    )
    parser.add_argument(
        "-T",
        "--threads",
        type=int,
        default=1,
        help="Number of threads to use when running tests in parallel. Default is 1.",
    )
    
    parser.add_argument(
        "-t",
        "--timeout",
        type=int,
        default=600,
        help="Per-test timeout in seconds (default: 600).",
    )

    args = parser.parse_args()
    # main(args.module, args.run_optional_tests, args.threads)

    os.environ["GLD_TEST_TIMEOUT"] = str(args.timeout)
    main(args.module, args.run_optional_tests, args.threads)

