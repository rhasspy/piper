#!/usr/bin/env bash

# Runs formatters, linters, and type checkers on Python code.

set -eo pipefail

# Directory of *this* script
this_dir="$( cd "$( dirname "$0" )" && pwd )"

base_dir="$(realpath "${this_dir}/..")"

# Path to virtual environment
: "${venv:=${base_dir}/.venv}"

if [ -d "${venv}" ]; then
    # Activate virtual environment if available
    source "${venv}/bin/activate"
fi

python_files=("${base_dir}/piper_train")

# Format code
black "${python_files[@]}"
isort "${python_files[@]}"

# Check
flake8 "${python_files[@]}"
pylint "${python_files[@]}"
mypy "${python_files[@]}"
