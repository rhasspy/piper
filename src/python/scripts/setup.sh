#!/usr/bin/env bash
set -eo pipefail

# Directory of *this* script
this_dir="$( cd "$( dirname "$0" )" && pwd )"

# Base directory of repo
base_dir="$(realpath "${this_dir}/..")"

# Path to virtual environment
: "${venv:=${base_dir}/.venv}"

# Python binary to use
: "${PYTHON=python3}"

python_version="$(${PYTHON} --version)"

# Create virtual environment
echo "Creating virtual environment at ${venv} (${python_version})"
rm -rf "${venv}"
"${PYTHON}" -m venv "${venv}"
source "${venv}/bin/activate"

# Install Python dependencies
echo 'Installing Python dependencies'
pip3 install --upgrade pip
pip3 install --upgrade wheel setuptools

pip3 install -r "${base_dir}/requirements.txt"

# -----------------------------------------------------------------------------

echo "OK"
