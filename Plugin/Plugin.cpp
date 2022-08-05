/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/

#include "../Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h"

#include <Logging.h>
#include <SystemToolbox.h>
#include <Toolbox.h>

#include <EmbeddedResources.h>

// we are using Orthanc 1.11.0 API (RequestedTags in tools/find)
#define ORTHANC_CORE_MINIMAL_MAJOR     1
#define ORTHANC_CORE_MINIMAL_MINOR     11
#define ORTHANC_CORE_MINIMAL_REVISION  0


std::unique_ptr<OrthancPlugins::OrthancConfiguration> orthancFullConfiguration_;
OrthancPlugins::OrthancConfiguration pluginConfiguration_(false);
Json::Value pluginJsonConfiguration_;
std::string oe2BaseUrl_;

template <enum Orthanc::EmbeddedResources::DirectoryResourceId folder>
void ServeEmbeddedFolder(OrthancPluginRestOutput* output,
                         const char* url,
                         const OrthancPluginHttpRequest* request)
{
  OrthancPluginContext* context = OrthancPlugins::GetGlobalContext();

  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context, output, "GET");
  }
  else
  {
    std::string path = "/" + std::string(request->groups[0]);
    Orthanc::MimeType mimeType = Orthanc::SystemToolbox::AutodetectMimeType(path);
    const char* mime = Orthanc::EnumerationToString(mimeType);

    std::string fileContent;
    Orthanc::EmbeddedResources::GetDirectoryResource(fileContent, folder, path.c_str());

    const char* resource = fileContent.size() ? fileContent.c_str() : NULL;
    OrthancPluginAnswerBuffer(context, output, resource, fileContent.size(), mime);
  }
}

template <enum Orthanc::EmbeddedResources::FileResourceId file, Orthanc::MimeType mime>
void ServeEmbeddedFile(OrthancPluginRestOutput* output,
                       const char* url,
                       const OrthancPluginHttpRequest* request)
{
  OrthancPluginContext* context = OrthancPlugins::GetGlobalContext();

  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context, output, "GET");
  }
  else
  {
    std::string s;
    Orthanc::EmbeddedResources::GetFileResource(s, file);

    const char* resource = s.size() ? s.c_str() : NULL;
    OrthancPluginAnswerBuffer(context, output, resource, s.size(), Orthanc::EnumerationToString(mime));
  }
}

void RedirectRoot(OrthancPluginRestOutput* output,
                  const char* url,
                  const OrthancPluginHttpRequest* request)
{
  OrthancPluginContext* context = OrthancPlugins::GetGlobalContext();

  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context, output, "GET");
  }
  else
  {
    for (uint32_t i = 0; i < request->headersCount; ++i)
    {
      OrthancPlugins::LogError(std::string(request->headersKeys[i]) + " : " + request->headersValues[i]);
    }

    std::string oe2BaseApp = oe2BaseUrl_ + "app/";
    OrthancPluginRedirect(context, output, &(oe2BaseApp.c_str()[1]));  // remove the first '/' to make a relative redirect !
  }
}

void MergeJson(Json::Value &a, const Json::Value &b) {                                                                        
                                                                                                                  
  if (!a.isObject() || !b.isObject())
  {
    return;
  }

  Json::Value::Members members = b.getMemberNames();

  for (size_t i = 0; i < members.size(); i++)
  {
    std::string key = members[i];
    
    if (!a[key].isNull() && a[key].type() == Json::objectValue && b[key].type() == Json::objectValue)
    {
      MergeJson(a[key], b[key]);
    } 
    else
    {
      // const std::string& val = b[key].asString();
      a[key] = b[key];
    }
  }
}


void ReadConfiguration()
{
  orthancFullConfiguration_.reset(new OrthancPlugins::OrthancConfiguration);

  // read default configuration
  std::string defaultConfigurationFileContent;
  Orthanc::EmbeddedResources::GetFileResource(defaultConfigurationFileContent, Orthanc::EmbeddedResources::DEFAULT_CONFIGURATION);

  Json::Value defaultConfiguration;
  OrthancPlugins::ReadJsonWithoutComments(defaultConfiguration, defaultConfigurationFileContent);
  pluginJsonConfiguration_ = defaultConfiguration["OrthancExplorer2"];

  if (orthancFullConfiguration_->IsSection("OrthancExplorer2"))
  {
    orthancFullConfiguration_->GetSection(pluginConfiguration_, "OrthancExplorer2");

    MergeJson(pluginJsonConfiguration_, pluginConfiguration_.GetJson());
  }
}

bool GetPluginConfiguration(Json::Value& pluginConfiguration, const std::string& sectionName)
{
  if (orthancFullConfiguration_->IsSection(sectionName))
  {
    OrthancPlugins::OrthancConfiguration pluginConfiguration_(false);    
    orthancFullConfiguration_->GetSection(pluginConfiguration_, sectionName);

    pluginConfiguration = pluginConfiguration_.GetJson();
    return true;
  }

  return false;
}


bool IsPluginEnabledInConfiguration(const std::string& sectionName, const std::string& enableValueName, bool defaultValue)
{
  if (orthancFullConfiguration_->IsSection(sectionName))
  {
    OrthancPlugins::OrthancConfiguration pluginConfiguration_(false);    
    orthancFullConfiguration_->GetSection(pluginConfiguration_, sectionName);

    return pluginConfiguration_.GetBooleanValue(enableValueName, defaultValue);
  }

  return defaultValue;
}

Json::Value GetPluginInfo(const std::string& pluginName)
{
  Json::Value pluginInfo;

  OrthancPlugins::RestApiGet(pluginInfo, "/plugins/" + pluginName, false);
  
  return pluginInfo;
}

Json::Value GetPluginsConfiguration()
{
  Json::Value pluginsConfiguration;
  Json::Value pluginList;

  Orthanc::UriComponents components;
  Orthanc::Toolbox::SplitUriComponents(components, oe2BaseUrl_);
  std::string pluginUriPrefix = "";  // the RootUri is provided relative to Orthanc Explorer /app/explorer.html -> we need to correct this !
  for (size_t i = 0; i < components.size(); i++)
  {
    pluginUriPrefix += "../";
  }

  OrthancPlugins::RestApiGet(pluginList, "/plugins", false);

  for (Json::Value::ArrayIndex i = 0; i < pluginList.size(); i++)
  {
    Json::Value pluginConfiguration;
    std::string pluginName = pluginList[i].asString();

    if (pluginName == "explorer.js")
    {
      continue;
    }

    Json::Value pluginInfo = GetPluginInfo(pluginName);

    if (pluginInfo.isMember("RootUri") && pluginInfo["RootUri"].asString().size() > 0)
    {
      pluginInfo["RootUri"] = pluginUriPrefix + pluginInfo["RootUri"].asString();
    }

    pluginsConfiguration[pluginName] = pluginInfo;
    pluginsConfiguration[pluginName]["Enabled"] = true;  // we assume that unknown plugins are enabled (if they are loaded by Orthanc)

    if (pluginName == "authorization") 
    {
      pluginsConfiguration[pluginName]["Enabled"] = GetPluginConfiguration(pluginConfiguration, "Authorization") && pluginConfiguration.isMember("WebService");
    }
    else if (pluginName == "connectivity-checks")
    {
      pluginsConfiguration[pluginName]["Enabled"] = true;
    }
    else if (pluginName == "delayed-deletion")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("DelayedDeletion", "Enable", false);
    }
    else if (pluginName == "dicom-web")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("DicomWeb", "Enable", false);
    }
    else if (pluginName == "gdcm")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("Gdcm", "Enable", true);
    }
    else if (pluginName == "mysql-index")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("MySQL", "EnableIndex", false);
    }
    else if (pluginName == "mysql-storage")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("MySQL", "EnableStorage", false);
    }
    else if (pluginName == "odbc-index")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("Odbc", "EnableIndex", false);
    }
    else if (pluginName == "odbc-storage")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("Odbc", "EnableStorage", false);
    }
    else if (pluginName == "postgresql-index")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("PostgreSQL", "EnableIndex", false);
    }
    else if (pluginName == "postgresql-storage")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("PostgreSQL", "EnableStorage", false);
    }
    else if (pluginName == "osimis-web-viewer")
    {
      pluginsConfiguration[pluginName]["Enabled"] = GetPluginConfiguration(pluginConfiguration, "WebViewer");
    }
    else if (pluginName == "python")
    {
      pluginsConfiguration[pluginName]["Enabled"] = GetPluginConfiguration(pluginConfiguration, "PythonScript");
    }
    else if (pluginName == "serve-folders")
    {
      pluginsConfiguration[pluginName]["Enabled"] = GetPluginConfiguration(pluginConfiguration, "ServeFolders");
    }
    else if (pluginName == "stone-webviewer")
    {
      pluginsConfiguration[pluginName]["Enabled"] = GetPluginConfiguration(pluginConfiguration, "StoneWebViewer");
    }
    else if (pluginName == "tcia")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("Tcia", "Enable", false);
    }
    else if (pluginName == "transfers")
    {
      pluginsConfiguration[pluginName]["Enabled"] = true;
    }
    else if (pluginName == "web-viewer")
    {
      pluginsConfiguration[pluginName]["Enabled"] = true;
    }
    else if (pluginName == "worklists")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("Worklists", "Enable", false);
    }
    else if (pluginName == "wsi")
    {
      pluginsConfiguration[pluginName]["Enabled"] = true;
    }
    

  }

  return pluginsConfiguration;
}


void GetOE2Configuration(OrthancPluginRestOutput* output,
                         const char* /*url*/,
                         const OrthancPluginHttpRequest* request)
{
  OrthancPluginContext* context = OrthancPlugins::GetGlobalContext();

  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context, output, "GET");
  }
  else
  {
    Json::Value oe2Configuration;
    oe2Configuration["UiOptions"] = pluginJsonConfiguration_["UiOptions"];
    oe2Configuration["Plugins"] = GetPluginsConfiguration();

    std::string answer = oe2Configuration.toStyledString();
    OrthancPluginAnswerBuffer(context, output, answer.c_str(), answer.size(), "application/json");
  }
}


static bool DisplayPerformanceWarning(OrthancPluginContext* context)
{
  (void) DisplayPerformanceWarning;   // Disable warning about unused function
  OrthancPluginLogWarning(context, "Performance warning in Orthanc Explorer 2: "
                          "Non-release build, runtime debug assertions are turned on");
  return true;
}

static void CheckRootUrlIsValid(const std::string& value, const std::string& name, bool allowEmpty)
{
  if (allowEmpty && value.size() == 0)
  {
    return;
  }

  if (value.size() < 1 ||
      value[0] != '/' ||
      value[value.size() - 1] != '/')
  {
    OrthancPlugins::LogError("Orthanc-Explorer 2: '" + name + "' configuration shall start with a '/' and end with a '/': " + value);
    throw Orthanc::OrthancException(Orthanc::ErrorCode_InternalError);
  }
}

extern "C"
{
  ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext* context)
  {
    assert(DisplayPerformanceWarning(context));

    OrthancPlugins::SetGlobalContext(context);
    
    Orthanc::Logging::InitializePluginContext(context);

    Orthanc::Logging::EnableInfoLevel(true);


    /* Check the version of the Orthanc core */
    if (!OrthancPlugins::CheckMinimalOrthancVersion(ORTHANC_CORE_MINIMAL_MAJOR,
                                                    ORTHANC_CORE_MINIMAL_MINOR,
                                                    ORTHANC_CORE_MINIMAL_REVISION))
    {
      OrthancPlugins::ReportMinimalOrthancVersion(ORTHANC_CORE_MINIMAL_MAJOR,
                                                  ORTHANC_CORE_MINIMAL_MINOR,
                                                  ORTHANC_CORE_MINIMAL_REVISION);
      return -1;
    }

    OrthancPluginSetDescription(context, "Advanced User Interface for Orthanc");

    try
    {
      ReadConfiguration();

      if (pluginJsonConfiguration_["Enable"].asBool())
      {
        oe2BaseUrl_ = pluginJsonConfiguration_["Root"].asString();

        CheckRootUrlIsValid(oe2BaseUrl_, "Root", false);

        OrthancPlugins::LogWarning("Root URI to the Orthanc-Explorer 2 application: " + oe2BaseUrl_);

        // we need to mix the "routing" between the server and the frontend (vue-router)
        // first part are the files that are 'static files' that must be served by the backend
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFolder<Orthanc::EmbeddedResources::WEB_APPLICATION_ASSETS> >
          (oe2BaseUrl_ + "app/assets/(.*)", true);
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_INDEX, Orthanc::MimeType_Html> >
          (oe2BaseUrl_ + "app/index.html", true);
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_FAVICON, Orthanc::MimeType_Ico> >
          (oe2BaseUrl_ + "app/favicon.ico", true);
        
        // second part are all the routes that are actually handled by vue-router and that are actually returning the same file (index.html)
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_INDEX, Orthanc::MimeType_Html> >
          (oe2BaseUrl_ + "app/(.*)", true);
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_INDEX, Orthanc::MimeType_Html> >
          (oe2BaseUrl_ + "app", true);

        OrthancPlugins::RegisterRestCallback<GetOE2Configuration>(oe2BaseUrl_ + "api/configuration", true);
        
        std::string pluginRootUri = oe2BaseUrl_ + "app/";
        OrthancPluginSetRootUri(context, pluginRootUri.c_str());

        if (pluginJsonConfiguration_["IsDefaultOrthancUI"].asBool())
        {
          OrthancPlugins::RegisterRestCallback<RedirectRoot>("/", true);
        }

      }
      else
      {
        OrthancPlugins::LogWarning("Orthanc Explorer 2 plugin is disabled");
      }
    }
    catch (Orthanc::OrthancException& e)
    {
      OrthancPlugins::LogError("Exception while initializing the Orthanc-Explorer 2 plugin: " + 
                               std::string(e.What()));
      return -1;
    }
    catch (...)
    {
      OrthancPlugins::LogError("Exception while initializing the Orthanc-Explorer 2 plugin");
      return -1;
    }

    return 0;
  }


  ORTHANC_PLUGINS_API void OrthancPluginFinalize()
  {
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetName()
  {
    return "orthanc-explorer-2";
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return ORTHANC_OE2_VERSION;
  }
}
