#!/usr/bin/env python3
"""
Test script for multilingual TTS functionality in Piper.
This script can be run locally to test various languages before CI/CD execution.
"""

import os
import sys
import subprocess
import tempfile
import json
import time
import argparse
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import urllib.request
import tarfile
import zipfile

# Language test configurations
LANGUAGE_CONFIGS = {
    "en_US": {
        "model": "en_US-lessac-medium",
        "test_text": "Hello, this is a test of the text to speech system.",
        "speaker": "lessac"
    },
    "en_GB": {
        "model": "en_GB-alan-medium",
        "test_text": "Good morning, this is a British English voice test.",
        "speaker": "alan"
    },
    "de_DE": {
        "model": "de_DE-thorsten-medium",
        "test_text": "Hallo, dies ist ein Test des Sprachsynthesesystems.",
        "speaker": "thorsten"
    },
    "fr_FR": {
        "model": "fr_FR-siwis-medium",
        "test_text": "Bonjour, ceci est un test du système de synthèse vocale.",
        "speaker": "siwis"
    },
    "es_ES": {
        "model": "es_ES-mls_9972-low",
        "test_text": "Hola, esta es una prueba del sistema de síntesis de voz.",
        "speaker": "mls_9972"
    },
    "it_IT": {
        "model": "it_IT-riccardo-x_low",
        "test_text": "Ciao, questo è un test del sistema di sintesi vocale.",
        "speaker": "riccardo"
    },
    "pt_BR": {
        "model": "pt_BR-faber-medium",
        "test_text": "Olá, este é um teste do sistema de síntese de voz.",
        "speaker": "faber"
    },
    "ru_RU": {
        "model": "ru_RU-dmitri-medium",
        "test_text": "Привет, это тест системы синтеза речи.",
        "speaker": "dmitri"
    },
    "zh_CN": {
        "model": "zh_CN-huayan-medium",
        "test_text": "你好，这是语音合成系统的测试。",
        "speaker": "huayan"
    },
    "nl_NL": {
        "model": "nl_NL-mls-medium",
        "test_text": "Hallo, dit is een test van het spraaksynthesesysteem.",
        "speaker": "mls"
    },
    "pl_PL": {
        "model": "pl_PL-gosia-medium",
        "test_text": "Witaj, to jest test systemu syntezy mowy.",
        "speaker": "gosia"
    },
    "sv_SE": {
        "model": "sv_SE-nst-medium",
        "test_text": "Hej, detta är ett test av talsyntessystemet.",
        "speaker": "nst"
    },
    "ar_JO": {
        "model": "ar_JO-kareem-medium",
        "test_text": "مرحبا، هذا اختبار لنظام تركيب الكلام.",
        "speaker": "kareem"
    },
    "cs_CZ": {
        "model": "cs_CZ-jirka-medium",
        "test_text": "Ahoj, toto je test systému syntézy řeči.",
        "speaker": "jirka"
    },
    "fi_FI": {
        "model": "fi_FI-harri-medium",
        "test_text": "Hei, tämä on puhesynteesijärjestelmän testi.",
        "speaker": "harri"
    },
    "hu_HU": {
        "model": "hu_HU-anna-medium",
        "test_text": "Helló, ez a beszédszintézis rendszer tesztje.",
        "speaker": "anna"
    },
    "no_NO": {
        "model": "no_NO-talesyntese-medium",
        "test_text": "Hei, dette er en test av talesyntesesystemet.",
        "speaker": "talesyntese"
    },
    "da_DK": {
        "model": "da_DK-talesyntese-medium",
        "test_text": "Hej, dette er en test af talesyntesesystemet.",
        "speaker": "talesyntese"
    },
    "el_GR": {
        "model": "el_GR-rapunzelina-low",
        "test_text": "Γεια σου, αυτή είναι μια δοκιμή του συστήματος σύνθεσης ομιλίας.",
        "speaker": "rapunzelina"
    },
    "tr_TR": {
        "model": "tr_TR-dfki-m-ailabs-medium",
        "test_text": "Merhaba, bu konuşma sentezi sisteminin bir testidir.",
        "speaker": "dfki"
    },
    "uk_UA": {
        "model": "uk_UA-lada-x_low",
        "test_text": "Привіт, це тест системи синтезу мовлення.",
        "speaker": "lada"
    },
    "vi_VN": {
        "model": "vi_VN-vivos-x_low",
        "test_text": "Xin chào, đây là bài kiểm tra hệ thống tổng hợp giọng nói.",
        "speaker": "vivos"
    },
    "ko_KR": {
        "model": "ko_KR-kss-x_low",
        "test_text": "안녕하세요, 이것은 음성 합성 시스템의 테스트입니다.",
        "speaker": "kss"
    }
}

# Model repository configuration
MODEL_REPO = "rhasspy/piper-voices"
MODEL_VERSION = "v1.0.0"


class MultilingualTTSTester:
    def __init__(self, piper_path: str, cache_dir: str = None):
        self.piper_path = Path(piper_path)
        self.cache_dir = Path(cache_dir or os.path.expanduser("~/.cache/piper/voices"))
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        
    def download_model(self, language: str, config: Dict) -> Tuple[Path, Path]:
        """Download model files for a specific language."""
        model_name = config["model"]
        speaker = config["speaker"]
        lang_prefix = language.split("_")[0]
        
        # Model paths
        model_dir = self.cache_dir / language
        model_dir.mkdir(parents=True, exist_ok=True)
        
        model_path = model_dir / f"{model_name}.onnx"
        config_path = model_dir / f"{model_name}.onnx.json"
        
        # Skip if already downloaded
        if model_path.exists() and config_path.exists():
            print(f"✓ Model {model_name} already cached")
            return model_path, config_path
        
        # Construct URLs
        quality = model_name.split("-")[-1]  # e.g., "medium", "low", "x_low"
        base_url = f"https://huggingface.co/{MODEL_REPO}/resolve/{MODEL_VERSION}/{lang_prefix}/{language}/{speaker}/{quality}"
        
        # Download model
        if not model_path.exists():
            model_url = f"{base_url}/{model_name}.onnx?download=true"
            print(f"Downloading {model_name}.onnx...")
            print(f"URL: {model_url}")
            try:
                urllib.request.urlretrieve(model_url, model_path)
            except Exception as e:
                print(f"Failed to download from primary URL: {e}")
                raise
        
        # Download config
        if not config_path.exists():
            config_url = f"{base_url}/{model_name}.onnx.json?download=true"
            print(f"Downloading {model_name}.onnx.json...")
            print(f"URL: {config_url}")
            try:
                urllib.request.urlretrieve(config_url, config_path)
            except Exception as e:
                print(f"Failed to download config from primary URL: {e}")
                raise
        
        print(f"✓ Downloaded {model_name} successfully")
        return model_path, config_path
    
    def test_language(self, language: str, config: Dict, 
                     test_type: str = "basic") -> Dict:
        """Test TTS for a specific language."""
        print(f"\n{'='*60}")
        print(f"Testing {language}: {config['model']}")
        print(f"{'='*60}")
        
        results = {
            "language": language,
            "model": config["model"],
            "status": "failed",
            "time": 0,
            "output_size": 0,
            "errors": []
        }
        
        try:
            # Download model
            model_path, config_path = self.download_model(language, config)
            
            # Create test input
            with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
                f.write(config["test_text"])
                input_file = f.name
            
            # Output file
            output_file = f"test_output_{language}.wav"
            
            # Run TTS
            print(f"Running TTS with text: {config['test_text'][:50]}...")
            start_time = time.time()
            
            cmd = [
                str(self.piper_path),
                "--model", str(model_path),
                "--output_file", output_file
            ]
            
            with open(input_file, 'r') as f:
                process = subprocess.run(
                    cmd,
                    stdin=f,
                    capture_output=True,
                    text=True
                )
            
            end_time = time.time()
            results["time"] = end_time - start_time
            
            # Check results
            if process.returncode == 0:
                if os.path.exists(output_file):
                    results["output_size"] = os.path.getsize(output_file)
                    if results["output_size"] > 10000:  # Minimum expected size
                        results["status"] = "success"
                        print(f"✓ Success! Generated {results['output_size']} bytes in {results['time']:.2f}s")
                    else:
                        results["errors"].append(f"Output file too small: {results['output_size']} bytes")
                else:
                    results["errors"].append("Output file not created")
            else:
                results["errors"].append(f"Process failed with code {process.returncode}")
                results["errors"].append(f"stderr: {process.stderr}")
            
            # Run additional tests based on test type
            if test_type in ["comprehensive", "performance"] and results["status"] == "success":
                self._run_additional_tests(language, config, model_path, test_type, results)
            
        except Exception as e:
            results["errors"].append(f"Exception: {str(e)}")
            print(f"✗ Error: {e}")
        finally:
            # Cleanup
            if 'input_file' in locals():
                os.unlink(input_file)
        
        return results
    
    def _run_additional_tests(self, language: str, config: Dict, 
                            model_path: Path, test_type: str, results: Dict):
        """Run additional tests for comprehensive or performance testing."""
        print("\nRunning additional tests...")
        
        # Test special characters
        special_tests = [
            "Testing numbers: 123, 456.78, -90",
            "Testing punctuation! Is it working? Yes... maybe.",
            "Testing symbols: $100, 50%, user@email.com",
            "Testing dates: January 1st, 2025 at 3:45 PM"
        ]
        
        for i, test_text in enumerate(special_tests):
            with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
                f.write(test_text)
                input_file = f.name
            
            output_file = f"test_special_{language}_{i}.wav"
            cmd = [
                str(self.piper_path),
                "--model", str(model_path),
                "--output_file", output_file
            ]
            
            with open(input_file, 'r') as f:
                subprocess.run(cmd, stdin=f, capture_output=True)
            
            os.unlink(input_file)
            if os.path.exists(output_file):
                print(f"  ✓ Special test {i+1} passed")
            else:
                print(f"  ✗ Special test {i+1} failed")
        
        # Performance test for long text
        if test_type == "performance":
            print("\nRunning performance test...")
            long_text = (config["test_text"] + " ") * 50  # Repeat 50 times
            
            with tempfile.NamedTemporaryFile(mode='w', suffix='.txt', delete=False) as f:
                f.write(long_text)
                input_file = f.name
            
            output_file = f"test_performance_{language}.wav"
            start_time = time.time()
            
            cmd = [
                str(self.piper_path),
                "--model", str(model_path),
                "--output_file", output_file
            ]
            
            with open(input_file, 'r') as f:
                subprocess.run(cmd, stdin=f, capture_output=True)
            
            end_time = time.time()
            perf_time = end_time - start_time
            
            os.unlink(input_file)
            if os.path.exists(output_file):
                size = os.path.getsize(output_file)
                print(f"  ✓ Performance: {len(long_text)} chars in {perf_time:.2f}s")
                print(f"    Speed: {len(long_text)/perf_time:.0f} chars/second")
                results["performance"] = {
                    "chars": len(long_text),
                    "time": perf_time,
                    "chars_per_second": len(long_text)/perf_time
                }
    
    def test_all_languages(self, languages: List[str] = None, 
                          test_type: str = "basic") -> Dict[str, Dict]:
        """Test all specified languages."""
        if languages is None:
            languages = list(LANGUAGE_CONFIGS.keys())
        
        results = {}
        for lang in languages:
            if lang in LANGUAGE_CONFIGS:
                results[lang] = self.test_language(lang, LANGUAGE_CONFIGS[lang], test_type)
            else:
                print(f"Warning: Unknown language {lang}")
        
        return results
    
    def print_summary(self, results: Dict[str, Dict]):
        """Print a summary of test results."""
        print(f"\n{'='*60}")
        print("TEST SUMMARY")
        print(f"{'='*60}")
        
        success_count = sum(1 for r in results.values() if r["status"] == "success")
        total_count = len(results)
        
        print(f"Total languages tested: {total_count}")
        print(f"Successful: {success_count}")
        print(f"Failed: {total_count - success_count}")
        print(f"Success rate: {success_count/total_count*100:.1f}%")
        
        print("\nDetailed Results:")
        print(f"{'Language':<10} {'Model':<25} {'Status':<10} {'Time':<8} {'Size':<10}")
        print("-" * 70)
        
        for lang, result in sorted(results.items()):
            status = "✓ Pass" if result["status"] == "success" else "✗ Fail"
            time_str = f"{result['time']:.2f}s" if result['time'] > 0 else "N/A"
            size_str = f"{result['output_size']/1024:.1f}KB" if result['output_size'] > 0 else "N/A"
            
            print(f"{lang:<10} {result['model']:<25} {status:<10} {time_str:<8} {size_str:<10}")
            
            if result["errors"]:
                for error in result["errors"]:
                    print(f"  → {error}")
        
        # Performance summary if available
        perf_results = {lang: r for lang, r in results.items() if "performance" in r}
        if perf_results:
            print(f"\n{'='*60}")
            print("PERFORMANCE SUMMARY")
            print(f"{'='*60}")
            print(f"{'Language':<10} {'Chars/Second':<15} {'Total Time':<10}")
            print("-" * 35)
            
            for lang, result in sorted(perf_results.items()):
                perf = result["performance"]
                print(f"{lang:<10} {perf['chars_per_second']:<15.0f} {perf['time']:<10.2f}s")


def main():
    parser = argparse.ArgumentParser(description="Test multilingual TTS with Piper")
    parser.add_argument("--piper", default="./piper/bin/piper",
                       help="Path to piper executable")
    parser.add_argument("--languages", nargs="+",
                       help="Languages to test (default: all)")
    parser.add_argument("--test-type", choices=["basic", "comprehensive", "performance"],
                       default="basic", help="Type of tests to run")
    parser.add_argument("--cache-dir", help="Directory to cache voice models")
    parser.add_argument("--skip-download", action="store_true",
                       help="Skip model downloads (use cached only)")
    
    args = parser.parse_args()
    
    # Check if piper exists
    if not os.path.exists(args.piper):
        print(f"Error: Piper not found at {args.piper}")
        sys.exit(1)
    
    # Initialize tester
    tester = MultilingualTTSTester(args.piper, args.cache_dir)
    
    # Run tests
    print(f"Starting multilingual TTS tests ({args.test_type} mode)...")
    results = tester.test_all_languages(args.languages, args.test_type)
    
    # Print summary
    tester.print_summary(results)
    
    # Return appropriate exit code
    success_count = sum(1 for r in results.values() if r["status"] == "success")
    if success_count == len(results):
        sys.exit(0)
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()