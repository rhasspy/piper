"""Utility for downloading Piper voices."""
import json
import logging
import shutil
from pathlib import Path
from typing import Any, Dict, Iterable, Set, Tuple, Union
from urllib.request import urlopen

from .file_hash import get_file_hash

URL_FORMAT = "https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/{file}"

_DIR = Path(__file__).parent
_LOGGER = logging.getLogger(__name__)

_SKIP_FILES = {"MODEL_CARD"}


class VoiceNotFoundError(Exception):
    pass


def get_voices() -> Dict[str, Any]:
    """Loads available voices from embedded JSON file."""
    with open(_DIR / "voices.json", "r", encoding="utf-8") as voices_file:
        return json.load(voices_file)


def ensure_voice_exists(
    name: str,
    data_dirs: Iterable[Union[str, Path]],
    download_dir: Union[str, Path],
    voices_info: Dict[str, Any],
):
    assert data_dirs, "No data dirs"
    if name not in voices_info:
        raise VoiceNotFoundError(name)

    voice_info = voices_info[name]
    voice_files = voice_info["files"]
    files_to_download: Set[str] = set()

    for data_dir in data_dirs:
        data_dir = Path(data_dir)

        # Check sizes/hashes
        for file_path, file_info in voice_files.items():
            if file_path in files_to_download:
                # Already planning to download
                continue

            file_name = Path(file_path).name
            if file_name in _SKIP_FILES:
                continue

            data_file_path = data_dir / file_name
            _LOGGER.debug("Checking %s", data_file_path)
            if not data_file_path.exists():
                _LOGGER.debug("Missing %s", data_file_path)
                files_to_download.add(file_path)
                continue

            expected_size = file_info["size_bytes"]
            actual_size = data_file_path.stat().st_size
            if expected_size != actual_size:
                _LOGGER.warning(
                    "Wrong size (expected=%s, actual=%s) for %s",
                    expected_size,
                    actual_size,
                    data_file_path,
                )
                files_to_download.add(file_path)
                continue

            expected_hash = file_info["md5_digest"]
            actual_hash = get_file_hash(data_file_path)
            if expected_hash != actual_hash:
                _LOGGER.warning(
                    "Wrong hash (expected=%s, actual=%s) for %s",
                    expected_hash,
                    actual_hash,
                    data_file_path,
                )
                files_to_download.add(file_path)
                continue

    if (not voice_files) and (not files_to_download):
        raise ValueError(f"Unable to find or download voice: {name}")

    # Download missing files
    download_dir = Path(download_dir)

    for file_path in files_to_download:
        file_name = Path(file_path).name
        if file_name in _SKIP_FILES:
            continue

        file_url = URL_FORMAT.format(file=file_path)
        download_file_path = download_dir / file_name
        download_file_path.parent.mkdir(parents=True, exist_ok=True)

        _LOGGER.debug("Downloading %s to %s", file_url, download_file_path)
        with urlopen(file_url) as response, open(
            download_file_path, "wb"
        ) as download_file:
            shutil.copyfileobj(response, download_file)

        _LOGGER.info("Downloaded %s (%s)", download_file_path, file_url)


def find_voice(name: str, data_dirs: Iterable[Union[str, Path]]) -> Tuple[Path, Path]:
    for data_dir in data_dirs:
        data_dir = Path(data_dir)
        onnx_path = data_dir / f"{name}.onnx"
        config_path = data_dir / f"{name}.onnx.json"

        if onnx_path.exists() and config_path.exists():
            return onnx_path, config_path

    raise ValueError(f"Missing files for voice {name}")
