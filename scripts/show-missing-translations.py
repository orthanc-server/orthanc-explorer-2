#!/usr/bin/python3

import pathlib
import json
import glob
import pprint

here = pathlib.Path(__file__).parent.resolve()

reference_file_path = str(here / '../WebApplication/src/locales/en.json')

with open(reference_file_path, 'rb') as f:
    reference_content = json.load(f)

language_files_path = glob.glob(str(here / '../WebApplication/src/locales/*.json'))


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
