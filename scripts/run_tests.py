#!/usr/bin/env python3
"""
Test runner script for piper-tts
Provides a unified interface for running all tests with various options
"""

import argparse
import sys
import subprocess
import os
from pathlib import Path

def run_command(cmd, cwd=None):
    """Run a command and return exit code"""
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd)
    return result.returncode

def main():
    parser = argparse.ArgumentParser(description="Run piper-tts tests")
    parser.add_argument("--unit", action="store_true", help="Run only unit tests")
    parser.add_argument("--integration", action="store_true", help="Run only integration tests")
    parser.add_argument("--python", action="store_true", help="Run only Python tests")
    parser.add_argument("--cpp", action="store_true", help="Run only C++ tests")
    parser.add_argument("--coverage", action="store_true", help="Generate coverage report")
    parser.add_argument("--parallel", action="store_true", help="Run tests in parallel")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--failfast", "-x", action="store_true", help="Stop on first failure")
    parser.add_argument("--japanese", action="store_true", help="Run only Japanese TTS tests")
    parser.add_argument("--no-gpu", action="store_true", help="Skip GPU tests")
    parser.add_argument("tests", nargs="*", help="Specific test files or directories")

    args = parser.parse_args()

    # Base directory
    base_dir = Path(__file__).parent

    # Determine what to test
    run_python = not args.cpp or args.python
    run_cpp = not args.python or args.cpp

    exit_code = 0

    # Run Python tests
    if run_python:
        print("\n=== Running Python Tests ===\n")

        pytest_args = ["python", "-m", "pytest"]

        if args.verbose:
            pytest_args.append("-v")

        if args.failfast:
            pytest_args.append("-x")

        if args.parallel:
            pytest_args.extend(["-n", "auto"])

        if args.coverage and not args.cpp:
            pytest_args.append("--cov")

        # Add markers
        markers = []
        if args.unit:
            markers.append("unit")
        if args.integration:
            markers.append("integration")
        if args.japanese:
            markers.append("japanese")
        if args.no_gpu:
            markers.append("not requires_gpu")

        if markers:
            pytest_args.extend(["-m", " and ".join(markers)])

        # Add specific test paths
        if args.tests:
            pytest_args.extend(args.tests)
        else:
            # Default test paths
            test_paths = []
            if (base_dir / "src/python/tests").exists():
                test_paths.append("src/python/tests")
            if (base_dir / "src/python_run/tests").exists():
                test_paths.append("src/python_run/tests")

            pytest_args.extend(test_paths)

        exit_code = run_command(pytest_args, cwd=base_dir)

    # Run C++ tests
    if run_cpp and exit_code == 0:
        print("\n=== Running C++ Tests ===\n")

        # Check if build directory exists
        build_dir = base_dir / "build"
        if not build_dir.exists():
            print("Build directory not found. Please build the project first.")
            return 1

        # Run CTest
        ctest_args = ["ctest"]

        if args.verbose:
            ctest_args.append("-V")
        else:
            ctest_args.append("--output-on-failure")

        if args.failfast:
            ctest_args.append("--stop-on-failure")

        if args.parallel:
            ctest_args.extend(["-j", str(os.cpu_count())])

        cpp_exit_code = run_command(ctest_args, cwd=build_dir)

        if cpp_exit_code != 0:
            exit_code = cpp_exit_code

    # Generate coverage report
    if args.coverage and run_python and exit_code == 0:
        print("\n=== Generating Coverage Report ===\n")
        run_command(["python", "-m", "coverage", "html"], cwd=base_dir)
        print(f"Coverage report generated in: {base_dir}/htmlcov/index.html")

    return exit_code

if __name__ == "__main__":
    sys.exit(main())