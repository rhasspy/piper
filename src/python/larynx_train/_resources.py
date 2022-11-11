"""Shared access to package resources"""
import json
import os
import typing
from pathlib import Path

try:
    import importlib.resources

    files = importlib.resources.files
except (ImportError, AttributeError):
    # Backport for Python < 3.9
    import importlib_resources  # type: ignore

    files = importlib_resources.files

_PACKAGE = "larynx_train"
_DIR = Path(typing.cast(os.PathLike, files(_PACKAGE)))

__version__ = (_DIR / "VERSION").read_text(encoding="utf-8").strip()
