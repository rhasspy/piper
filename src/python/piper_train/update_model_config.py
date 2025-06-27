#!/usr/bin/env python3
"""
Update existing Piper model configurations to use PUA phoneme mappings.

This script modifies model JSON configuration files to replace multi-character
phonemes with their corresponding Private Use Area (PUA) single-character
representations, ensuring compatibility between Python training and C++ inference.
"""

import json
import argparse
import shutil
from pathlib import Path
from typing import Dict, List, Any
from piper_train.phonemize.token_mapper import FIXED_PUA_MAPPING, TOKEN2CHAR, CHAR2TOKEN


def update_phoneme_id_map(config: Dict[str, Any]) -> bool:
    """
    Update the phoneme_id_map in a model configuration to use PUA characters.
    
    Args:
        config: The model configuration dictionary
        
    Returns:
        bool: True if any changes were made, False otherwise
    """
    if "phoneme_id_map" not in config:
        print("Warning: No phoneme_id_map found in configuration")
        return False
    
    phoneme_id_map = config["phoneme_id_map"]
    new_phoneme_id_map = {}
    changes_made = False
    
    # Process each phoneme in the map
    for phoneme, ids in phoneme_id_map.items():
        # Check if this is a multi-character phoneme that needs PUA mapping
        if phoneme in FIXED_PUA_MAPPING:
            # Replace with PUA character
            pua_char = TOKEN2CHAR[phoneme]
            new_phoneme_id_map[pua_char] = ids
            changes_made = True
            print(f"  Mapped: '{phoneme}' -> U+{ord(pua_char):04X} ('{pua_char}')")
        else:
            # Keep single-character phonemes as-is
            new_phoneme_id_map[phoneme] = ids
    
    # Replace the phoneme_id_map
    config["phoneme_id_map"] = new_phoneme_id_map
    
    return changes_made


def process_model_config(config_path: Path, backup: bool = True) -> None:
    """
    Process a single model configuration file.
    
    Args:
        config_path: Path to the JSON configuration file
        backup: Whether to create a backup of the original file
    """
    print(f"\nProcessing: {config_path}")
    
    # Read the configuration
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            config = json.load(f)
    except Exception as e:
        print(f"Error reading {config_path}: {e}")
        return
    
    # Check if this is a Japanese model
    phoneme_type = config.get("phoneme_type", config.get("espeak", {}).get("voice", ""))
    if "openjtalk" not in phoneme_type.lower() and "ja" not in str(config_path).lower():
        print("  Skipping: Not a Japanese model")
        return
    
    # Create backup if requested
    if backup:
        backup_path = config_path.with_suffix('.json.bak')
        shutil.copy2(config_path, backup_path)
        print(f"  Created backup: {backup_path}")
    
    # Update the phoneme mappings
    if update_phoneme_id_map(config):
        # Write the updated configuration
        with open(config_path, 'w', encoding='utf-8') as f:
            json.dump(config, f, ensure_ascii=False, indent=2)
        print("  Configuration updated successfully")
    else:
        print("  No changes needed")


def main():
    parser = argparse.ArgumentParser(
        description="Update Piper model configurations to use PUA phoneme mappings"
    )
    parser.add_argument(
        "configs",
        nargs="+",
        type=Path,
        help="Path(s) to model configuration JSON files"
    )
    parser.add_argument(
        "--no-backup",
        action="store_true",
        help="Don't create backup files"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be changed without modifying files"
    )
    
    args = parser.parse_args()
    
    print("PUA Phoneme Mapping Update Tool")
    print("=" * 40)
    print("\nFixed PUA mappings:")
    for phoneme, codepoint in sorted(FIXED_PUA_MAPPING.items()):
        print(f"  {phoneme:3s} -> U+{codepoint:04X}")
    
    # Process each configuration file
    for config_path in args.configs:
        if not config_path.exists():
            print(f"\nError: {config_path} does not exist")
            continue
        
        if not config_path.suffix == '.json':
            print(f"\nWarning: {config_path} is not a JSON file, skipping")
            continue
        
        if args.dry_run:
            print(f"\n[DRY RUN] Would process: {config_path}")
            # Just show what would be done
            with open(config_path, 'r', encoding='utf-8') as f:
                config = json.load(f)
            phoneme_id_map = config.get("phoneme_id_map", {})
            for phoneme in phoneme_id_map:
                if phoneme in FIXED_PUA_MAPPING:
                    pua_char = TOKEN2CHAR[phoneme]
                    print(f"  Would map: '{phoneme}' -> U+{ord(pua_char):04X}")
        else:
            process_model_config(config_path, backup=not args.no_backup)


if __name__ == "__main__":
    main()