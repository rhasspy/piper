#!/usr/bin/env python3
"""
Check available voice models on HuggingFace
"""

import requests
import json

def check_models():
    # Base URL
    base_url = "https://huggingface.co/api/models"
    
    # Search for piper voice models
    params = {
        "search": "rhasspy piper-voices",
        "limit": 100
    }
    
    response = requests.get(base_url, params=params)
    if response.status_code == 200:
        models = response.json()
        print(f"Found {len(models)} models")
        
        # Look for specific language models
        languages = ["es_ES", "nl_NL", "fr_FR", "de_DE", "en_US", "it_IT", "pt_BR", "ru_RU", "zh_CN"]
        
        for lang in languages:
            print(f"\n=== {lang} ===")
            # Try different URL patterns
            test_urls = [
                f"https://huggingface.co/rhasspy/piper-voices/tree/main/{lang[:2]}/{lang}",
                f"https://huggingface.co/rhasspy/piper-voices/resolve/v1.0.0/{lang[:2]}/{lang}/",
            ]
            
            for url in test_urls:
                resp = requests.head(url, allow_redirects=True)
                print(f"{url}: {resp.status_code}")
    else:
        print(f"Failed to fetch models: {response.status_code}")

if __name__ == "__main__":
    check_models()