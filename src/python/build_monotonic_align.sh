#!/usr/bin/env bash
set -eo pipefail

this_dir="$( cd "$( dirname "$0" )" && pwd )"

if [ -d "${this_dir}/.venv" ]; then
    source "${this_dir}/.venv/bin/activate"
fi

cd "${this_dir}/piper_train/vits/monotonic_align"
mkdir -p monotonic_align
cythonize -i core.pyx
mv core*.so monotonic_align/
