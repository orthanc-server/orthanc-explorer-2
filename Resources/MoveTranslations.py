import json
import os
import shutil
import pathlib

here = pathlib.Path(__file__).parent.resolve() / "../WebApplication/src/locales/"

for p in here.glob("*.json"):
    with open(p, "r", encoding="utf-8") as f:
        translations = json.load(f)

        if "settings" in translations and isinstance(translations["settings"], str):
            title = translations["settings"]
            del translations["settings"]
            translations["settings"] = {}

        for i in ["dicom_AET", 
                  "dicom_port", 
                  "ingest_transcoding", 
                  "installed_plugins", 
                  "orthanc_name", 
                  "orthanc_system_info", 
                  "orthanc_version", 
                  "overwrite_instances", 
                  "plugins_not_enabled", 
                  "striked_through", 
                  "ingest_transcoding", 
                  "verbosity_level", 
                  "storage_compression", 
                  "storage_size", 
                  "statistics"
                  ]:
            if i in translations:
                translations["settings"][i] = translations[i]
                del translations[i]

    with open(p, "w", encoding="utf-8") as f:
        data = json.dump(translations, f, ensure_ascii=False, indent=4, sort_keys=True)
