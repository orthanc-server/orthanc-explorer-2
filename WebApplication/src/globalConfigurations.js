// this file contains variables that can be updated by the plugin (C++ code) before
// serving the static file.  This is a way to update the base urls everywhere in the HTML/JS

export var baseOe2Url = '${OE2_BASE_URL}';  // will be updated by the plugin
if (baseOe2Url.includes('OE2_BASE_URL')) {  // if not updated by the plugin, set a default value for dev mode
  baseOe2Url = "/ui/app"
}

export var orthancApiUrl = '${ORTHANC_API_BASE_URL}';  // will be updated by the plugin
if (orthancApiUrl.includes('ORTHANC_API_BASE_URL')) {  // if not updated by the plugin, set a default value for dev mode
    orthancApiUrl = "/"
}

export var oe2ApiUrl = '${OE2_API_BASE_URL}';  // will be updated by the plugin
if (oe2ApiUrl.includes('OE2_API_BASE_URL')) {  // if not updated by the plugin, set a default value for dev mode
    oe2ApiUrl = "/ui/api/"
}

