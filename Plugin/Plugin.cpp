/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2024 Osimis S.A., Belgium
 * Copyright (C) 2021-2024 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 * Copyright (C) 2024-2024 Orthanc Team SRL, Belgium
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
#include <SerializationToolbox.h>

#include <EmbeddedResources.h>

#define ORTHANC_PLUGIN_NAME  "orthanc-explorer-2"

// we are using Orthanc 1.11.0 API (RequestedTags in tools/find)
#define ORTHANC_CORE_MINIMAL_MAJOR     1
#define ORTHANC_CORE_MINIMAL_MINOR     11
#define ORTHANC_CORE_MINIMAL_REVISION  0


std::unique_ptr<OrthancPlugins::OrthancConfiguration> orthancFullConfiguration_;
Json::Value pluginJsonConfiguration_;
std::string oe2BaseUrl_;

Json::Value pluginsConfiguration_;
bool hasUserProfile_ = false;
bool openInOhifV3IsExplicitelyDisabled = false;
bool enableShares_ = false;
bool isReadOnly_ = false;
bool hasAuditLogs_ = false;
std::string customCssPath_;
std::string theme_ = "light";
std::string customLogoPath_;
std::string customLogoUrl_;
std::string customFavIconPath_;
std::string customTitle_;

enum CustomFilesPath
{
  CustomFilesPath_Logo,
  CustomFilesPath_FavIcon
};


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

    if (file == Orthanc::EmbeddedResources::WEB_APPLICATION_INDEX && theme_ != "light")
    {
      boost::replace_all(s, "data-bs-theme=\"light\"", "data-bs-theme=\"" + theme_ + "\"");
    }

    const char* resource = s.size() ? s.c_str() : NULL;
    OrthancPluginAnswerBuffer(context, output, resource, s.size(), Orthanc::EnumerationToString(mime));
  }
}

template <enum CustomFilesPath customFile> 
void ServeCustomFile(OrthancPluginRestOutput* output,
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
    std::string fileContent;
    std::string customFileContent;
    std::string customFilePath;
    if (customFile == CustomFilesPath_FavIcon)
    {
      customFilePath = customFavIconPath_;
    }
    else if (customFile == CustomFilesPath_Logo)
    {
      customFilePath = customLogoPath_;
    }
    else
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_NotImplemented);
    }

    Orthanc::SystemToolbox::ReadFile(fileContent, customFilePath);
    Orthanc::MimeType mimeType = Orthanc::SystemToolbox::AutodetectMimeType(customFilePath);

    // include an ETag for correct cache handling
    OrthancPlugins::OrthancString md5;
    size_t size = fileContent.size();
    md5.Assign(OrthancPluginComputeMd5(OrthancPlugins::GetGlobalContext(), fileContent.c_str(), size));

    std::string etag = "\"" + std::string(md5.GetContent()) + "\"";
    OrthancPluginSetHttpHeader(OrthancPlugins::GetGlobalContext(), output, "ETag", etag.c_str());
    OrthancPluginSetHttpHeader(OrthancPlugins::GetGlobalContext(), output, "Cache-Control", "no-cache");

    OrthancPluginAnswerBuffer(context, output, fileContent.c_str(), size, Orthanc::EnumerationToString(mimeType));
  }
}

// serves either the default CSS or a custom file CSS
void ServeCustomCss(OrthancPluginRestOutput* output,
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
    std::string cssFileContent;

    if (strstr(url, "custom.css") != NULL)
    {
      if (theme_ == "dark")
      {
        Orthanc::EmbeddedResources::GetFileResource(cssFileContent, Orthanc::EmbeddedResources::DEFAULT_CSS_DARK);
      }
      else
      {
        Orthanc::EmbeddedResources::GetFileResource(cssFileContent, Orthanc::EmbeddedResources::DEFAULT_CSS_LIGHT);
      }

      if (!customCssPath_.empty())
      { // append the custom CSS
        std::string customCssFileContent;
        Orthanc::SystemToolbox::ReadFile(customCssFileContent, customCssPath_);
        cssFileContent += "\n/* Appending the custom CSS */\n" + customCssFileContent;
      }
    }

    const char* resource = cssFileContent.size() ? cssFileContent.c_str() : NULL;
    size_t size = cssFileContent.size();

    // include an ETag for correct cache handling
    OrthancPlugins::OrthancString md5;
    md5.Assign(OrthancPluginComputeMd5(OrthancPlugins::GetGlobalContext(), resource, size));

    std::string etag = "\"" + std::string(md5.GetContent()) + "\"";
    OrthancPluginSetHttpHeader(OrthancPlugins::GetGlobalContext(), output, "ETag", etag.c_str());
    OrthancPluginSetHttpHeader(OrthancPlugins::GetGlobalContext(), output, "Cache-Control", "no-cache");

    OrthancPluginAnswerBuffer(context, output, resource, size, Orthanc::EnumerationToString(Orthanc::MimeType_Css));
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
    OrthancPlugins::OrthancConfiguration pluginConfiguration(false);
    orthancFullConfiguration_->GetSection(pluginConfiguration, "OrthancExplorer2");

    Json::Value jsonConfig = pluginConfiguration.GetJson();

    // backward compatibility  
    if (jsonConfig.isMember("UiOptions"))
    {
      // fix typo from version 0.7.0
      if (jsonConfig["UiOptions"].isMember("EnableAnonimization") && !jsonConfig["UiOptions"].isMember("EnableAnonymization"))
      {
        LOG(WARNING) << "You are still using the 'UiOptions.EnableAnonimization' configuration that has a typo.  You should use 'UiOptions.EnableAnonymization' instead.";
        jsonConfig["UiOptions"]["EnableAnonymization"] = jsonConfig["UiOptions"]["EnableAnonimization"];
      }

      if (jsonConfig["UiOptions"].isMember("StudyListEmptyIfNoSearch") && !jsonConfig["UiOptions"].isMember("StudyListContentIfNoSearch"))
      {
        if (jsonConfig["UiOptions"]["StudyListEmptyIfNoSearch"].asBool() == true)
        {
          LOG(WARNING) << "You are still using the 'UiOptions.StudyListEmptyIfNoSearch' configuration that is now deprecated a typo.  You should use 'UiOptions.StudyListContentIfNoSearch' instead.";
          jsonConfig["UiOptions"]["StudyListContentIfNoSearch"] = "empty";
        }
      }

      openInOhifV3IsExplicitelyDisabled = jsonConfig["UiOptions"].isMember("EnableOpenInOhifViewer3") && jsonConfig["UiOptions"]["EnableOpenInOhifViewer3"].asBool() == false;
    }

    MergeJson(pluginJsonConfiguration_, jsonConfig);

    if (jsonConfig.isMember("CustomCssPath") && jsonConfig["CustomCssPath"].isString())
    {
      customCssPath_ = jsonConfig["CustomCssPath"].asString();
      if (!Orthanc::SystemToolbox::IsExistingFile(customCssPath_))
      {
        LOG(ERROR) << "Unable to accesss the 'CustomCssPath': " << customCssPath_;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentFile);
      }
    }

    if (jsonConfig.isMember("CustomLogoPath") && jsonConfig["CustomLogoPath"].isString())
    {
      customLogoPath_ = jsonConfig["CustomLogoPath"].asString();
      if (!Orthanc::SystemToolbox::IsExistingFile(customLogoPath_))
      {
        LOG(ERROR) << "Unable to accesss the 'CustomLogoPath': " << customLogoPath_;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentFile);
      }
    }

    if (jsonConfig.isMember("CustomLogoUrl") && jsonConfig["CustomLogoUrl"].isString())
    {
      customLogoUrl_ = jsonConfig["CustomLogoUrl"].asString();
    }

    if (jsonConfig.isMember("Theme") && jsonConfig["Theme"].isString() && jsonConfig["Theme"].asString() == "dark")
    {
      theme_ = "dark";
    }

    if (jsonConfig.isMember("CustomFavIconPath") && jsonConfig["CustomFavIconPath"].isString())
    {
      customFavIconPath_ = jsonConfig["CustomFavIconPath"].asString();
      if (!Orthanc::SystemToolbox::IsExistingFile(customFavIconPath_))
      {
        LOG(ERROR) << "Unable to accesss the 'CustomFavIconPath': " << customFavIconPath_;
        throw Orthanc::OrthancException(Orthanc::ErrorCode_InexistentFile);
      }
    }

    if (jsonConfig.isMember("CustomTitle") && jsonConfig["CustomTitle"].isString())
    {
      customTitle_ = jsonConfig["CustomTitle"].asString();
    }
  }

  enableShares_ = pluginJsonConfiguration_["UiOptions"]["EnableShares"].asBool(); // we are sure that the value exists since it is in the default configuration file
  
  isReadOnly_ = orthancFullConfiguration_->GetBooleanValue("ReadOnly", false);

  if (orthancFullConfiguration_->IsSection("Authorization"))
  {
    OrthancPlugins::OrthancConfiguration authPluginConfiguration(false);
    orthancFullConfiguration_->GetSection(authPluginConfiguration, "Authorization");

    hasAuditLogs_ = authPluginConfiguration.GetBooleanValue("EnableAuditLogs", false);
  }
}

bool GetPluginConfiguration(Json::Value& jsonPluginConfiguration, const std::string& sectionName)
{
  if (orthancFullConfiguration_->IsSection(sectionName))
  {
    OrthancPlugins::OrthancConfiguration pluginConfiguration(false);    
    orthancFullConfiguration_->GetSection(pluginConfiguration, sectionName);

    jsonPluginConfiguration = pluginConfiguration.GetJson();
    return true;
  }

  return false;
}


bool IsPluginEnabledInConfiguration(const std::string& sectionName, const std::string& enableValueName, bool defaultValue)
{
  if (orthancFullConfiguration_->IsSection(sectionName))
  {
    OrthancPlugins::OrthancConfiguration pluginConfiguration(false);    
    orthancFullConfiguration_->GetSection(pluginConfiguration, sectionName);

    return pluginConfiguration.GetBooleanValue(enableValueName, defaultValue);
  }

  return defaultValue;
}

Json::Value GetPluginInfo(const std::string& pluginName)
{
  Json::Value pluginInfo;

  OrthancPlugins::RestApiGet(pluginInfo, "/plugins/" + pluginName, false);
  
  return pluginInfo;
}

Json::Value GetKeycloakConfiguration()
{
  if (pluginJsonConfiguration_.isMember("Keycloak"))
  {
    const Json::Value& keyCloakSection = pluginJsonConfiguration_["Keycloak"];
    if (keyCloakSection.isMember("Enable") && keyCloakSection["Enable"].asBool() == true)
    {
      return pluginJsonConfiguration_["Keycloak"];
    }
  }

  return Json::nullValue;
}

Json::Value GetTokenLandingConfiguration()
{
  if (pluginJsonConfiguration_.isMember("Tokens") && pluginJsonConfiguration_["Tokens"].isMember("LandingOptions"))
  {
    return pluginJsonConfiguration_["Tokens"]["LandingOptions"];
  }

  return Json::nullValue;
}

Json::Value GetInboxConfiguration()
{
  if (pluginJsonConfiguration_.isMember("Inbox"))
  {
    if (pluginJsonConfiguration_["Inbox"].isMember("Enable") && pluginJsonConfiguration_["Inbox"]["Enable"].asBool())
    {
      return pluginJsonConfiguration_["Inbox"];
    }
  }

  return Json::nullValue;
}

Json::Value GetPluginsConfiguration(bool& hasUserProfile)
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
      pluginConfiguration = Json::nullValue;
      pluginsConfiguration[pluginName]["Enabled"] = GetPluginConfiguration(pluginConfiguration, "Authorization") 
                                                    && (pluginConfiguration.isMember("WebService") 
                                                        || pluginConfiguration.isMember("WebServiceRootUrl")
                                                        || pluginConfiguration.isMember("WebServiceUserProfileUrl")
                                                        || pluginConfiguration.isMember("WebServiceTokenValidationUrl")
                                                        || pluginConfiguration.isMember("WebServiceTokenCreationBaseUrl")
                                                        || pluginConfiguration.isMember("WebServiceTokenDecoderUrl"));
      hasUserProfile = GetPluginConfiguration(pluginConfiguration, "Authorization") && (pluginConfiguration.isMember("WebServiceUserProfileUrl") || pluginConfiguration.isMember("WebServiceRootUrl"));

      if (!pluginConfiguration.isMember("CheckedLevel") || pluginConfiguration["CheckedLevel"].asString() != "studies")
      {
        LOG(WARNING) << "When using OE2 and the authorization plugin together, you must set 'Authorization.CheckedLevel' to 'studies'.  Unless you are using this orthanc only to generate tokens.";
      }
    }
    else if (pluginName == "advanced-storage")
    {
      pluginsConfiguration[pluginName]["Enabled"] = IsPluginEnabledInConfiguration("AdvancedStorage", "Enable", false);
    }
    else if (pluginName == "AWS S3 Storage")
    {
      pluginsConfiguration[pluginName]["Enabled"] = GetPluginConfiguration(pluginConfiguration, "AwsS3Storage");
    }
    else if (pluginName == "Azure Blob Storage")
    {
      pluginsConfiguration[pluginName]["Enabled"] = GetPluginConfiguration(pluginConfiguration, "AzureBlobStorage");
    }
    else if (pluginName == "connectivity-checks")
    {
      pluginsConfiguration[pluginName]["Enabled"] = true;
    }
    else if (pluginName == "ohif")
    {
      pluginsConfiguration[pluginName]["Enabled"] = true;
      std::string ohifDataSource = "dicom-web";
      if (GetPluginConfiguration(pluginConfiguration, "OHIF"))
      {
        if (pluginConfiguration.isMember("DataSource") && pluginConfiguration["DataSource"].asString() == "dicom-json")
        {
          ohifDataSource = "dicom-json";
        }
      }
      pluginsConfiguration[pluginName]["DataSource"] = ohifDataSource;
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
    else if (pluginName == "Google Cloud Storage")
    {
      pluginsConfiguration[pluginName]["Enabled"] = GetPluginConfiguration(pluginConfiguration, "GoogleCloudStorage");
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
      std::string notUsed;
      pluginsConfiguration[pluginName]["Enabled"] = orthancFullConfiguration_->LookupStringValue(notUsed, "PythonScript");
    }
    else if (pluginName == "serve-folders")
    {
      pluginsConfiguration[pluginName]["Enabled"] = GetPluginConfiguration(pluginConfiguration, "ServeFolders");
    }
    else if (pluginName == "stone-webviewer")
    {
      pluginsConfiguration[pluginName]["Enabled"] = true;
    }
    else if (pluginName == "volview")
    {
      pluginsConfiguration[pluginName]["Enabled"] = true;
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
    else if (pluginName == "multitenant-dicom")
    {
      pluginsConfiguration[pluginName]["Enabled"] = false;
      Json::Value pluginConfiguration;
      if (GetPluginConfiguration(pluginConfiguration, "MultitenantDicom"))
      {
        pluginsConfiguration[pluginName]["Enabled"] = pluginConfiguration.isMember("Servers") && pluginConfiguration["Servers"].isArray() && pluginConfiguration["Servers"].size() > 0;
      }
    }

  }

  return pluginsConfiguration;
}

void UpdateUiOptions(Json::Value& uiOption, const std::list<std::string>& permissions, const std::string& anyOfPermissions)
{
  std::vector<std::string> permissionsVector;
  Orthanc::Toolbox::TokenizeString(permissionsVector, anyOfPermissions, '|');

  bool hasPermission = false;

  for (size_t i = 0; i < permissionsVector.size(); ++i)
  {
    hasPermission |= std::find(permissions.begin(), permissions.end(), permissionsVector[i]) != permissions.end();
  }

  uiOption = uiOption.asBool() && hasPermission;
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

    oe2Configuration["Plugins"] = pluginsConfiguration_;
    oe2Configuration["UiOptions"] = pluginJsonConfiguration_["UiOptions"];
    
    // if OHIF has not been explicitely disabled in the config and if the plugin is loaded, enable it
    if (!openInOhifV3IsExplicitelyDisabled && pluginsConfiguration_.isMember("ohif"))
    {
      oe2Configuration["UiOptions"]["EnableOpenInOhifViewer3"] = true;
    }

    Json::Value tokens = pluginJsonConfiguration_["Tokens"];
    if (!tokens.isMember("RequiredForLinks"))
    {
      tokens["RequiredForLinks"] = hasUserProfile_;
    }

    oe2Configuration["Tokens"] = tokens;

    oe2Configuration["AdvancedOptions"] = pluginJsonConfiguration_["AdvancedOptions"];

    oe2Configuration["HasCustomLogo"] = !customLogoPath_.empty() || !customLogoUrl_.empty();
    if (!customLogoUrl_.empty())
    {
      oe2Configuration["CustomLogoUrl"] = customLogoUrl_;
    }

    if (!customTitle_.empty())
    {
      oe2Configuration["CustomTitle"] = customTitle_;
    }

    Json::Value& uiOptions = oe2Configuration["UiOptions"];

    if (hasUserProfile_)
    {
      {// get the available-labels from the auth plugin (and the auth-service)
        std::map<std::string, std::string> headers;
        OrthancPlugins::GetHttpHeaders(headers, request);

        uiOptions["EnablePermissionsEdition"] = false;

        Json::Value rolesConfig;
        if (OrthancPlugins::RestApiGet(rolesConfig, "/auth/settings/roles", headers, true))
        {
          if (rolesConfig.isObject() && rolesConfig.isMember("available-labels"))
          {
            LOG(INFO) << "Overriding \"AvailableLabels\" in UiOptions with the values from the auth-service";
            uiOptions["AvailableLabels"] = rolesConfig["available-labels"];
          }

          LOG(INFO) << rolesConfig.toStyledString();

          // if the auth-service is not fully configured, disable permissions edition
          if (rolesConfig.isObject() && rolesConfig.isMember("roles") && rolesConfig["roles"].isObject() && rolesConfig["roles"].size() > 0)
          {
            uiOptions["EnablePermissionsEdition"] = true;
          }
        }
      }

      {// get the user profile from the auth plugin (and the auth-service)
        std::map<std::string, std::string> headers;
        OrthancPlugins::GetHttpHeaders(headers, request);

        Json::Value userProfile;
        OrthancPlugins::RestApiGet(userProfile, "/auth/user/profile", headers, true);

        // modify the UiOptions based on the user profile
        std::list<std::string> permissions;
        Orthanc::SerializationToolbox::ReadListOfStrings(permissions, userProfile, "permissions");

        LOG(INFO) << "Overriding \"Enable...\" in UiOptions with the permissions from the auth-service for this user-profile";

        UpdateUiOptions(uiOptions["EnableStudyList"], permissions, "all|view");
        UpdateUiOptions(uiOptions["EnableViewerQuickButton"], permissions, "all|view");
        UpdateUiOptions(uiOptions["EnableReportQuickButton"], permissions, "all|view");
        UpdateUiOptions(uiOptions["EnableUpload"], permissions, "all|upload");
        UpdateUiOptions(uiOptions["EnableAuditLogs"], permissions, "admin-permissions|audit-logs");
        UpdateUiOptions(uiOptions["EnableAddSeries"], permissions, "all|upload");
        UpdateUiOptions(uiOptions["EnableDicomModalities"], permissions, "all|q-r-remote-modalities");
        UpdateUiOptions(uiOptions["EnableDeleteResources"], permissions, "all|delete");
        UpdateUiOptions(uiOptions["EnableDownloadZip"], permissions, "all|download");
        UpdateUiOptions(uiOptions["EnableDownloadDicomDir"], permissions, "all|download");
        UpdateUiOptions(uiOptions["EnableDownloadDicomFile"], permissions, "all|download");
        UpdateUiOptions(uiOptions["EnableModification"], permissions, "all|modify");
        UpdateUiOptions(uiOptions["EnableAnonymization"], permissions, "all|anonymize");
        UpdateUiOptions(uiOptions["EnableSendTo"], permissions, "all|send");
        UpdateUiOptions(uiOptions["EnableApiViewMenu"], permissions, "all|api-view");
        UpdateUiOptions(uiOptions["EnableSettings"], permissions, "all|settings");
        UpdateUiOptions(uiOptions["EnableShares"], permissions, "all|share");
        UpdateUiOptions(uiOptions["EnableEditLabels"], permissions, "all|edit-labels");
        UpdateUiOptions(uiOptions["EnablePermissionsEdition"], permissions, "admin-permissions");

        // the Legacy UI is not available with user profile since it would not refresh the tokens
        uiOptions["EnableLinkToLegacyUi"] = false;

        oe2Configuration["Profile"] = userProfile;
      }
    }

    bool adaptUiOnReadOnlySystems = oe2Configuration["AdvancedOptions"]["AdaptUiOnReadOnlySystems"].asBool();

    // disable operations on read only systems
    if (isReadOnly_ && adaptUiOnReadOnlySystems)
    {
      uiOptions["EnableUpload"] = false;
      uiOptions["EnableAddSeries"] = false;
      uiOptions["EnableDeleteResources"] = false;
      uiOptions["EnableModification"] = false;
      uiOptions["EnableAnonymization"] = false;
      uiOptions["EnableEditLabels"] = false;
      uiOptions["EnablePermissionsEdition"] = false;
    }

    oe2Configuration["Keycloak"] = GetKeycloakConfiguration();

    uiOptions["EnableAuditLogs"] = uiOptions["EnableAuditLogs"].asBool() && hasAuditLogs_;

    std::string answer = oe2Configuration.toStyledString();
    OrthancPluginAnswerBuffer(context, output, answer.c_str(), answer.size(), "application/json");
  }
}

void GetOE2PreLoginConfiguration(OrthancPluginRestOutput* output,
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
    oe2Configuration["Keycloak"] = GetKeycloakConfiguration();
    oe2Configuration["TokensLandingOptions"] = GetTokenLandingConfiguration();
    oe2Configuration["Inbox"] = GetInboxConfiguration();

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

OrthancPluginErrorCode OnChangeCallback(OrthancPluginChangeType changeType,
                                        OrthancPluginResourceType resourceType,
                                        const char* resourceId)
{
  try
  {
    if (changeType == OrthancPluginChangeType_OrthancStarted)
    {
      // this can not be performed during plugin initialization because it is accessing the DB -> must be done when Orthanc has just started
      pluginsConfiguration_ = GetPluginsConfiguration(hasUserProfile_);
    }
  }
  catch (Orthanc::OrthancException& e)
  {
    LOG(ERROR) << "Exception: " << e.What();
    return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
  }

  return OrthancPluginErrorCode_Success;
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

    OrthancPlugins::SetDescription(ORTHANC_PLUGIN_NAME, "Advanced User Interface for Orthanc");

    try
    {
      ReadConfiguration();

      if (pluginJsonConfiguration_["Enable"].asBool())
      {
        oe2BaseUrl_ = pluginJsonConfiguration_["Root"].asString();

        CheckRootUrlIsValid(oe2BaseUrl_, "Root", false);

        OrthancPlugins::LogWarning("Root URI to the Orthanc-Explorer 2 application: " + oe2BaseUrl_);


        OrthancPlugins::RegisterRestCallback
          <ServeCustomCss>
          (oe2BaseUrl_ + "app/customizable/custom.css", true);

        if (!customLogoPath_.empty())
        {
          OrthancPlugins::RegisterRestCallback
            <ServeCustomFile<CustomFilesPath_Logo> >
            (oe2BaseUrl_ + "app/customizable/custom-logo", true);
        }

        // we need to mix the "routing" between the server and the frontend (vue-router)
        // first part are the files that are 'static files' that must be served by the backend
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFolder<Orthanc::EmbeddedResources::WEB_APPLICATION_ASSETS> >
          (oe2BaseUrl_ + "app/assets/(.*)", true);
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_INDEX, Orthanc::MimeType_Html> >
          (oe2BaseUrl_ + "app/index.html", true);
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_INDEX_LANDING, Orthanc::MimeType_Html> >
          (oe2BaseUrl_ + "app/token-landing.html", true);
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_INDEX_RETRIEVE_AND_VIEW, Orthanc::MimeType_Html> >
          (oe2BaseUrl_ + "app/retrieve-and-view.html", true);
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_INBOX, Orthanc::MimeType_Html> >
          (oe2BaseUrl_ + "app/inbox.html", true);
        
        if (customFavIconPath_.empty())
        {
          OrthancPlugins::RegisterRestCallback
            <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_FAVICON, Orthanc::MimeType_Ico> >
            (oe2BaseUrl_ + "app/favicon.ico", true);
        }
        else
        {
          OrthancPlugins::RegisterRestCallback
            <ServeCustomFile<CustomFilesPath_FavIcon> >
            (oe2BaseUrl_ + "app/favicon.ico", true);
        }        
        // second part are all the routes that are actually handled by vue-router and that are actually returning the same file (index.html)
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_INDEX, Orthanc::MimeType_Html> >
          (oe2BaseUrl_ + "app/(.*)", true);
        OrthancPlugins::RegisterRestCallback
          <ServeEmbeddedFile<Orthanc::EmbeddedResources::WEB_APPLICATION_INDEX, Orthanc::MimeType_Html> >
          (oe2BaseUrl_ + "app", true);

        OrthancPlugins::RegisterRestCallback<GetOE2Configuration>(oe2BaseUrl_ + "api/configuration", true);
        OrthancPlugins::RegisterRestCallback<GetOE2PreLoginConfiguration>(oe2BaseUrl_ + "api/pre-login-configuration", true);

        std::string pluginRootUri = oe2BaseUrl_ + "app/";
        OrthancPlugins::SetRootUri(ORTHANC_PLUGIN_NAME, pluginRootUri.c_str());

        if (pluginJsonConfiguration_["IsDefaultOrthancUI"].asBool())
        {
          OrthancPlugins::RegisterRestCallback<RedirectRoot>("/", true);
        }

        OrthancPluginRegisterOnChangeCallback(context, OnChangeCallback);

        {
          std::string explorer;
          Orthanc::EmbeddedResources::GetFileResource(explorer, Orthanc::EmbeddedResources::ORTHANC_EXPLORER);

          std::map<std::string, std::string> dictionary;
          dictionary["OE2_BASE_URL"] = oe2BaseUrl_.substr(1, oe2BaseUrl_.size() - 2);  // Remove heading and trailing slashes
          std::string explorerConfigured = Orthanc::Toolbox::SubstituteVariables(explorer, dictionary);

          OrthancPluginExtendOrthancExplorer(OrthancPlugins::GetGlobalContext(), explorerConfigured.c_str());
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
    return ORTHANC_PLUGIN_NAME;
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return ORTHANC_OE2_VERSION;
  }
}
