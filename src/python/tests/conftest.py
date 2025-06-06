import sys
from pathlib import Path

# Add <repo>/src/python to PYTHONPATH during tests so that
# `import piper_train ...` works when tests are executed from project root.
_current = Path(__file__).resolve()
# Walk up until we find a directory that contains "src/python"
python_src = None
for parent in _current.parents:
    candidate = parent / "src" / "python"
    if candidate.is_dir():
        python_src = candidate
        break

if python_src and (str(python_src) not in sys.path):
    sys.path.insert(0, str(python_src)) 