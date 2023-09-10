#!/usr/bin/env python3
from pathlib import Path

import setuptools
from setuptools import setup

this_dir = Path(__file__).parent
module_dir = this_dir / "piper"

requirements = []
requirements_path = this_dir / "requirements.txt"
if requirements_path.is_file():
    with open(requirements_path, "r", encoding="utf-8") as requirements_file:
        requirements = requirements_file.read().splitlines()

data_files = [module_dir / "voices.json"]

# -----------------------------------------------------------------------------

setup(
    name="piper-tts",
    version="1.2.0",
    description="A fast, local neural text to speech system that sounds great and is optimized for the Raspberry Pi 4.",
    url="http://github.com/rhasspy/piper",
    author="Michael Hansen",
    author_email="mike@rhasspy.org",
    license="MIT",
    packages=setuptools.find_packages(),
    package_data={"piper": [str(p.relative_to(module_dir)) for p in data_files]},
    entry_points={
        "console_scripts": [
            "piper = piper.__main__:main",
        ]
    },
    install_requires=requirements,
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Topic :: Text Processing :: Linguistic",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
    ],
    keywords="rhasspy piper tts",
)
