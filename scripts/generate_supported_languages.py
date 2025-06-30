#!/usr/bin/env python3
from dataclasses import dataclass


@dataclass
class Language:
    native: str
    english: str
    country: str


_LANGUAGES = {
    "ar_JO": Language("العربية", "Arabic", "Jordan"),
    "ca_ES": Language("Català", "Catalan", "Spain"),
    "cs_CZ": Language("Čeština", "Czech", "Czech Republic"),
    "cy_GB": Language("Cymraeg", "Welsh", "Great Britain"),
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
    "lv_LV": Language("Latviešu", "Latvian", "Latvia"),
    "ne_NP": Language("नेपाली", "Nepali", "Nepal"),
    "nl_BE": Language("Nederlands", "Dutch", "Belgium"),
    "nl_NL": Language("Nederlands", "Dutch", "Netherlands"),
    "no_NO": Language("Norsk", "Norwegian", "Norway"),
    "ml_IN": Language("മലയാളം", "Malayalam", "India"),
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

def main() -> None:
    for lang_code, lang in sorted(_LANGUAGES.items()):
        print("*", f"{lang.native}, {lang.country}", f"({lang.english}, {lang_code})")


if __name__ == "__main__":
    main()
