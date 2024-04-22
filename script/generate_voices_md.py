#!/usr/bin/env python3
import argparse
import logging
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path


@dataclass
class Voice:
    lang_family: str
    lang_code: str
    dataset: str
    quality: str
    model_url: str
    config_url: str


@dataclass
class Language:
    native: str
    english: str
    country: str


_LANGUAGES = {
    "ar_JO": Language("العربية", "Arabic", "Jordan"),
    "ca_ES": Language("Català", "Catalan", "Spain"),
    "cs_CZ": Language("Čeština", "Czech", "Czech Republic"),
    "da_DK": Language("Dansk", "Danish", "Denmark"),
    "de_DE": Language("Deutsch", "German", "Germany"),
    "el_GR": Language("Ελληνικά", "Greek", "Greece"),
    "en_GB": Language("English", "English", "Great Britain"),
    "en_US": Language("English", "English", "United States"),
    "es_ES": Language("Español", "Spanish", "Spain"),
    "es_MX": Language("Español", "Spanish", "Mexico"),
    "fa_IR": Language("فارسی", "Farsi", "Iran"),
    "fi_FI": Language("Suomi", "Finnish", "Finland"),
    "fr_FR": Language("Français", "French", "France"),
    "is_IS": Language("íslenska", "Icelandic", "Iceland"),
    "it_IT": Language("Italiano", "Italian", "Italy"),
    "hu_HU": Language("Magyar", "Hungarian", "Hungary"),
    "ka_GE": Language("ქართული ენა", "Georgian", "Georgia"),
    "kk_KZ": Language("қазақша", "Kazakh", "Kazakhstan"),
    "lb_LU": Language("Lëtzebuergesch", "Luxembourgish", "Luxembourg"),
    "ne_NP": Language("नेपाली", "Nepali", "Nepal"),
    "nl_BE": Language("Nederlands", "Dutch", "Belgium"),
    "nl_NL": Language("Nederlands", "Dutch", "Netherlands"),
    "no_NO": Language("Norsk", "Norwegian", "Norway"),
    "pl_PL": Language("Polski", "Polish", "Poland"),
    "pt_BR": Language("Português", "Portuguese", "Brazil"),
    "pt_PT": Language("Português", "Portuguese", "Portugal"),
    "ro_RO": Language("Română", "Romanian", "Romania"),
    "ru_RU": Language("Русский", "Russian", "Russia"),
    "sk_SK": Language("Slovenčina", "Slovak", "Slovakia"),
    "sl_SI": Language("Slovenščina", "Slovenian", "Slovenia"),
    "sr_RS": Language("srpski", "Serbian", "Serbia"),
    "sv_SE": Language("Svenska", "Swedish", "Sweden"),
    "sw_CD": Language("Kiswahili", "Swahili", "Democratic Republic of the Congo"),
    "tr_TR": Language("Türkçe", "Turkish", "Turkey"),
    "uk_UA": Language("украї́нська мо́ва", "Ukrainian", "Ukraine"),
    "vi_VN": Language("Tiếng Việt", "Vietnamese", "Vietnam"),
    "zh_CN": Language("简体中文", "Chinese", "China"),
}

_QUALITY = {"x_low": 0, "low": 1, "medium": 2, "high": 3}


_LOGGER = logging.getLogger()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--piper-voices", required=True, help="Path to piper-voices root"
    )
    parser.add_argument(
        "--model-url-format",
        default="https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/{lang_family}/{lang_code}/{dataset}/{quality}/{lang_code}-{dataset}-{quality}.onnx?download=true",
        help="URL format for models with lang_family, lang_code, dataset, and quality",
    )
    parser.add_argument(
        "--config-url-format",
        default="https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/{lang_family}/{lang_code}/{dataset}/{quality}/{lang_code}-{dataset}-{quality}.onnx.json?download=true",
        help="URL format for configs with lang_family, lang_code, dataset, and quality",
    )
    args = parser.parse_args()
    logging.basicConfig(level=logging.DEBUG)

    voices_by_lang_code = defaultdict(list)
    piper_voices = Path(args.piper_voices)
    for onnx_path in piper_voices.rglob("*.onnx"):
        _LOGGER.debug(onnx_path)
        parts = onnx_path.stem.split("-")
        if len(parts) != 3:
            _LOGGER.warning("Skipping %s", onnx_path)
            continue

        lang_code, dataset, quality = parts
        assert lang_code in _LANGUAGES, f"Missing {lang_code}"
        lang_family = lang_code.split("_")[0]
        model_url = args.model_url_format.format(
            lang_family=lang_family,
            lang_code=lang_code,
            dataset=dataset,
            quality=quality,
        )
        config_url = args.config_url_format.format(
            lang_family=lang_family,
            lang_code=lang_code,
            dataset=dataset,
            quality=quality,
        )

        voices_by_lang_code[lang_code].append(
            Voice(
                lang_family=lang_family,
                lang_code=lang_code,
                dataset=dataset,
                quality=quality,
                model_url=model_url,
                config_url=config_url,
            )
        )

    print("# Voices")
    print("")

    for lang_code in sorted(voices_by_lang_code):
        lang_info = _LANGUAGES[lang_code]
        if lang_code.startswith("en_"):
            print("*", lang_info.english, f"({lang_code})")
        else:
            print("*", lang_info.english, f"(`{lang_code}`, {lang_info.native})")

        last_dataset = None
        for voice in sorted(
            voices_by_lang_code[lang_code],
            key=lambda v: (v.dataset, _QUALITY[v.quality]),
        ):
            if voice.dataset != last_dataset:
                print("    *", voice.dataset)
                last_dataset = voice.dataset

            print(
                "        *",
                voice.quality,
                "-",
                f"[[model]({voice.model_url})]",
                f"[[config]({voice.config_url}.json)]",
            )


if __name__ == "__main__":
    main()
