#!/usr/bin/python3

import pathlib
import json
import glob
import pprint

here = pathlib.Path(__file__).parent.resolve()

reference_file_path = str(here / '../WebApplication/src/locales/en.json')
language_files_path = glob.glob(str(here / '../WebApplication/src/locales/*.json'))
source_files_path = glob.glob(str(here / '../WebApplication/src/**/*.vue'), recursive=True)

with open(reference_file_path, 'rb') as f:
    reference_content = json.load(f)

# check if all entries in the default language file are being used
all_source_files_content = ""
for source_file in source_files_path:
    with open(source_file, "rt") as f:
        all_source_files_content += f.read()

# collect all keys
all_keys = []
def add_keys(node, prefix, all_keys):
    for key in node:
        if isinstance(node[key], dict):
            add_keys(node[key], prefix=f"{key}.", all_keys=all_keys)
        else:
            all_keys.append(f"{prefix}{key}")

add_keys(reference_content, "", all_keys=all_keys)

for key in all_keys:
    translations_text = [
        f'"{key}"',
        f"'{key}'"
    ]
    found = False
    for translation_text in translations_text:
        if translation_text in all_source_files_content:
            found = True
            break

    if not found:
        print(f"translation '{key}' does not seem to be used")


def compare_json_node(reference, value, prefix, missings = [], excessives = []):
    for key in reference:
        if not key in value:
            missings.append(f"{prefix}{key}")
        
        elif isinstance(reference[key], dict):
            compare_json_node(reference[key], value[key], prefix=f"{key}.", missings=missings, excessives=excessives)
    for key in value:
        if not key in reference:
            excessives.append(f"{prefix}{key}")


for path in language_files_path:
    if path != reference_file_path:
        with open(path, 'rb') as f:
            language_content = json.load(f)

            missings = []
            excessives = []
            
            compare_json_node(reference_content, language_content, prefix="", missings=missings, excessives=excessives)

            if len(missings) > 0:
                print(f">>> {path}, missing nodes in translation:")
                for missing in missings:
                    print(f"  {missing}")

            if len(excessives) > 0:
                print(f">>> {path}, unused nodes in translation:")
                for excessive in excessives:
                    print(f"  {excessive}")

            if len(missings) == 0 and len(excessives) == 0:
                print(f">>> {path}, translation is complete !")
