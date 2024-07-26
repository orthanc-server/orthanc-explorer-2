/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2024-2024 Orthanc Team SRL, Belgium
 * Copyright (C) 2021-2024 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "OrthancPluginCppWrapper.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/move/unique_ptr.hpp>
#include <boost/thread.hpp>


#include <json/reader.h>
#include <json/version.h>
#include <json/writer.h>

#if !defined(JSONCPP_VERSION_MAJOR) || !defined(JSONCPP_VERSION_MINOR)
#  error Cannot access the version of JsonCpp
#endif


/**
 * We use deprecated "Json::Reader", "Json::StyledWriter" and
 * "Json::FastWriter" if JsonCpp < 1.7.0. This choice is rather
 * arbitrary, but if Json >= 1.9.0, gcc generates explicit deprecation
 * warnings (clang was warning in earlier versions). For reference,
 * these classes seem to have been deprecated since JsonCpp 1.4.0 (on
 * February 2015) by the following changeset:
 * https://github.com/open-source-parsers/jsoncpp/commit/8df98f6112890d6272734975dd6d70cf8999bb22
 **/
#if (JSONCPP_VERSION_MAJOR >= 2 ||                                      \
     (JSONCPP_VERSION_MAJOR == 1 && JSONCPP_VERSION_MINOR >= 8))
#  define JSONCPP_USE_DEPRECATED 0
#else
#  define JSONCPP_USE_DEPRECATED 1
#endif


#if !ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 2, 0)
static const OrthancPluginErrorCode OrthancPluginErrorCode_NullPointer = OrthancPluginErrorCode_Plugin;
#endif


namespace OrthancPlugins
{
  static OrthancPluginContext* globalContext_ = NULL;
  static std::string pluginName_;


  void SetGlobalContext(OrthancPluginContext* context)
  {
    if (context == NULL)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(NullPointer);
    }
    else if (globalContext_ == NULL)
    {
      globalContext_ = context;
    }
    else
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(BadSequenceOfCalls);
    }
  }


  void SetGlobalContext(OrthancPluginContext* context,
                        const char* pluginName)
  {
    SetGlobalContext(context);
    pluginName_ = pluginName;
  }


  void ResetGlobalContext()
  {
    globalContext_ = NULL;
    pluginName_.clear();
  }

  bool HasGlobalContext()
  {
    return globalContext_ != NULL;
  }


  OrthancPluginContext* GetGlobalContext()
  {
    if (globalContext_ == NULL)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(BadSequenceOfCalls);
    }
    else
    {
      return globalContext_;
    }
  }


#if HAS_ORTHANC_PLUGIN_LOG_MESSAGE == 1
  void LogMessage(OrthancPluginLogLevel level,
                  const char* file,
                  uint32_t line,
                  const std::string& message)
  {
    if (HasGlobalContext())
    {
#if HAS_ORTHANC_PLUGIN_LOG_MESSAGE == 1
      const char* pluginName = (pluginName_.empty() ? NULL : pluginName_.c_str());
      OrthancPluginLogMessage(GetGlobalContext(), message.c_str(), pluginName, file, line, OrthancPluginLogCategory_Generic, level);
#else
      switch (level)
      {
        case OrthancPluginLogLevel_Error:
          OrthancPluginLogError(GetGlobalContext(), message.c_str());
          break;

        case OrthancPluginLogLevel_Warning:
          OrthancPluginLogWarning(GetGlobalContext(), message.c_str());
          break;

        case OrthancPluginLogLevel_Info:
          OrthancPluginLogInfo(GetGlobalContext(), message.c_str());
          break;

        default:
          ORTHANC_PLUGINS_THROW_EXCEPTION(ParameterOutOfRange);
      }
#endif
    }
  }
#endif


  void LogError(const std::string& message)
  {
    if (HasGlobalContext())
    {
      OrthancPluginLogError(GetGlobalContext(), message.c_str());
    }
  }

  void LogWarning(const std::string& message)
  {
    if (HasGlobalContext())
    {
      OrthancPluginLogWarning(GetGlobalContext(), message.c_str());
    }
  }

  void LogInfo(const std::string& message)
  {
    if (HasGlobalContext())
    {
      OrthancPluginLogInfo(GetGlobalContext(), message.c_str());
    }
  }


  void MemoryBuffer::Check(OrthancPluginErrorCode code)
  {
    if (code != OrthancPluginErrorCode_Success)
    {
      // Prevent using garbage information
      buffer_.data = NULL;
      buffer_.size = 0;
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(code);
    }
  }


  bool MemoryBuffer::CheckHttp(OrthancPluginErrorCode code)
  {
    if (code != OrthancPluginErrorCode_Success)
    {
      // Prevent using garbage information
      buffer_.data = NULL;
      buffer_.size = 0;
    }

    if (code == OrthancPluginErrorCode_Success)
    {
      return true;
    }
    else if (code == OrthancPluginErrorCode_UnknownResource ||
             code == OrthancPluginErrorCode_InexistentItem)
    {
      return false;
    }
    else
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(code);
    }
  }


  MemoryBuffer::MemoryBuffer()
  {
    buffer_.data = NULL;
    buffer_.size = 0;
  }


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
  MemoryBuffer::MemoryBuffer(const void* buffer,
                             size_t size)
  {
    uint32_t s = static_cast<uint32_t>(size);
    if (static_cast<size_t>(s) != size)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(NotEnoughMemory);
    }
    else if (OrthancPluginCreateMemoryBuffer(GetGlobalContext(), &buffer_, s) !=
             OrthancPluginErrorCode_Success)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(NotEnoughMemory);
    }
    else
    {
      memcpy(buffer_.data, buffer, size);
    }
  }
#endif


  void MemoryBuffer::Clear()
  {
    if (buffer_.data != NULL)
    {
      OrthancPluginFreeMemoryBuffer(GetGlobalContext(), &buffer_);
      buffer_.data = NULL;
      buffer_.size = 0;
    }
  }


  void MemoryBuffer::Assign(OrthancPluginMemoryBuffer& other)
  {
    Clear();

    buffer_.data = other.data;
    buffer_.size = other.size;

    other.data = NULL;
    other.size = 0;
  }


  void MemoryBuffer::Swap(MemoryBuffer& other)
  {
    std::swap(buffer_.data, other.buffer_.data);
    std::swap(buffer_.size, other.buffer_.size);
  }


  OrthancPluginMemoryBuffer MemoryBuffer::Release()
  {
    OrthancPluginMemoryBuffer result = buffer_;

    buffer_.data = NULL;
    buffer_.size = 0;

    return result;
  }


  void MemoryBuffer::ToString(std::string& target) const
  {
    if (buffer_.size == 0)
    {
      target.clear();
    }
    else
    {
      target.assign(reinterpret_cast<const char*>(buffer_.data), buffer_.size);
    }
  }


  void MemoryBuffer::ToJson(Json::Value& target) const
  {
    if (buffer_.data == NULL ||
        buffer_.size == 0)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    if (!ReadJson(target, buffer_.data, buffer_.size))
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot convert some memory buffer to JSON");
      ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
    }
  }


  bool MemoryBuffer::RestApiGet(const std::string& uri,
                                bool applyPlugins)
  {
    Clear();

    if (applyPlugins)
    {
      return CheckHttp(OrthancPluginRestApiGetAfterPlugins(GetGlobalContext(), &buffer_, uri.c_str()));
    }
    else
    {
      return CheckHttp(OrthancPluginRestApiGet(GetGlobalContext(), &buffer_, uri.c_str()));
    }
  }

  // helper class to convert std::map of headers to the plugin SDK C structure
  class PluginHttpHeaders
  {
  private:
    std::vector<const char*> headersKeys_;
    std::vector<const char*> headersValues_;

  public:
    explicit PluginHttpHeaders(const std::map<std::string, std::string>& httpHeaders)
    {
      for (std::map<std::string, std::string>::const_iterator
             it = httpHeaders.begin(); it != httpHeaders.end(); ++it)
      {
        headersKeys_.push_back(it->first.c_str());
        headersValues_.push_back(it->second.c_str());
      }      
    }

    const char* const* GetKeys()
    {
      return (headersKeys_.empty() ? NULL : &headersKeys_[0]);
    }

    const char* const* GetValues()
    {
      return (headersValues_.empty() ? NULL : &headersValues_[0]);
    }

    uint32_t GetSize()
    {
      return static_cast<uint32_t>(headersKeys_.size());
    }
  };

  bool MemoryBuffer::RestApiGet(const std::string& uri,
                                const std::map<std::string, std::string>& httpHeaders,
                                bool applyPlugins)
  {
    Clear();

    PluginHttpHeaders headers(httpHeaders);

    return CheckHttp(OrthancPluginRestApiGet2(
                       GetGlobalContext(), &buffer_, uri.c_str(), 
                       headers.GetSize(),
                       headers.GetKeys(),
                       headers.GetValues(), applyPlugins));
  }

  bool MemoryBuffer::RestApiPost(const std::string& uri,
                                 const void* body,
                                 size_t bodySize,
                                 bool applyPlugins)
  {
    Clear();
    
    // Cast for compatibility with Orthanc SDK <= 1.5.6
    const char* b = reinterpret_cast<const char*>(body);

    if (applyPlugins)
    {
      return CheckHttp(OrthancPluginRestApiPostAfterPlugins(GetGlobalContext(), &buffer_, uri.c_str(), b, bodySize));
    }
    else
    {
      return CheckHttp(OrthancPluginRestApiPost(GetGlobalContext(), &buffer_, uri.c_str(), b, bodySize));
    }
  }

#if HAS_ORTHANC_PLUGIN_GENERIC_CALL_REST_API == 1

  bool MemoryBuffer::RestApiPost(const std::string& uri,
                                 const void* body,
                                 size_t bodySize,
                                 const std::map<std::string, std::string>& httpHeaders,
                                 bool applyPlugins)
  {
    MemoryBuffer answerHeaders;
    uint16_t httpStatus;

    PluginHttpHeaders headers(httpHeaders);

    return CheckHttp(OrthancPluginCallRestApi(GetGlobalContext(), 
                                              &buffer_,
                                              *answerHeaders,
                                              &httpStatus,
                                              OrthancPluginHttpMethod_Post,
                                              uri.c_str(),
                                              headers.GetSize(), headers.GetKeys(), headers.GetValues(),
                                              body, bodySize,
                                              applyPlugins));
  }


  bool MemoryBuffer::RestApiPost(const std::string& uri,
                                 const Json::Value& body,
                                 const std::map<std::string, std::string>& httpHeaders,
                                 bool applyPlugins)
  {
    std::string s;
    WriteFastJson(s, body);
    return RestApiPost(uri, s.c_str(), s.size(), httpHeaders, applyPlugins);
  }
#endif

  bool MemoryBuffer::RestApiPut(const std::string& uri,
                                const void* body,
                                size_t bodySize,
                                bool applyPlugins)
  {
    Clear();

    // Cast for compatibility with Orthanc SDK <= 1.5.6
    const char* b = reinterpret_cast<const char*>(body);

    if (applyPlugins)
    {
      return CheckHttp(OrthancPluginRestApiPutAfterPlugins(GetGlobalContext(), &buffer_, uri.c_str(), b, bodySize));
    }
    else
    {
      return CheckHttp(OrthancPluginRestApiPut(GetGlobalContext(), &buffer_, uri.c_str(), b, bodySize));
    }
  }


  static bool ReadJsonInternal(Json::Value& target,
                               const void* buffer,
                               size_t size,
                               bool collectComments)
  {
#if JSONCPP_USE_DEPRECATED == 1
    Json::Reader reader;
    return reader.parse(reinterpret_cast<const char*>(buffer),
                        reinterpret_cast<const char*>(buffer) + size, target, collectComments);
#else
    Json::CharReaderBuilder builder;
    builder.settings_["collectComments"] = collectComments;
    
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    assert(reader.get() != NULL);
    
    JSONCPP_STRING err;
    if (reader->parse(reinterpret_cast<const char*>(buffer),
                      reinterpret_cast<const char*>(buffer) + size, &target, &err))
    {
      return true;
    }
    else
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot parse JSON: " + std::string(err));
      return false;
    }
#endif
  }


  bool ReadJson(Json::Value& target,
                const std::string& source)
  {
    return ReadJson(target, source.empty() ? NULL : source.c_str(), source.size());
  }
  

  bool ReadJson(Json::Value& target,
                const void* buffer,
                size_t size)
  {
    return ReadJsonInternal(target, buffer, size, true);
  }
  

  bool ReadJsonWithoutComments(Json::Value& target,
                               const std::string& source)
  {
    return ReadJsonWithoutComments(target, source.empty() ? NULL : source.c_str(), source.size());
  }
  

  bool ReadJsonWithoutComments(Json::Value& target,
                               const void* buffer,
                               size_t size)
  {
    return ReadJsonInternal(target, buffer, size, false);
  }


  void WriteFastJson(std::string& target,
                     const Json::Value& source)
  {
#if JSONCPP_USE_DEPRECATED == 1
    Json::FastWriter writer;
    target = writer.write(source);
#else
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "";
    target = Json::writeString(builder, source);
#endif
  }
  

  void WriteStyledJson(std::string& target,
                       const Json::Value& source)
  {
#if JSONCPP_USE_DEPRECATED == 1
    Json::StyledWriter writer;
    target = writer.write(source);
#else
    Json::StreamWriterBuilder builder;
    builder.settings_["indentation"] = "   ";
    target = Json::writeString(builder, source);
#endif
  }


  bool MemoryBuffer::RestApiPost(const std::string& uri,
                                 const Json::Value& body,
                                 bool applyPlugins)
  {
    std::string s;
    WriteFastJson(s, body);
    return RestApiPost(uri, s, applyPlugins);
  }


  bool MemoryBuffer::RestApiPut(const std::string& uri,
                                const Json::Value& body,
                                bool applyPlugins)
  {
    std::string s;
    WriteFastJson(s, body);
    return RestApiPut(uri, s, applyPlugins);
  }


  void MemoryBuffer::CreateDicom(const Json::Value& tags,
                                 OrthancPluginCreateDicomFlags flags)
  {
    Clear();

    std::string s;
    WriteFastJson(s, tags);

    Check(OrthancPluginCreateDicom(GetGlobalContext(), &buffer_, s.c_str(), NULL, flags));
  }

  void MemoryBuffer::CreateDicom(const Json::Value& tags,
                                 const OrthancImage& pixelData,
                                 OrthancPluginCreateDicomFlags flags)
  {
    Clear();

    std::string s;
    WriteFastJson(s, tags);

    Check(OrthancPluginCreateDicom(GetGlobalContext(), &buffer_, s.c_str(), pixelData.GetObject(), flags));
  }


  void MemoryBuffer::ReadFile(const std::string& path)
  {
    Clear();
    Check(OrthancPluginReadFile(GetGlobalContext(), &buffer_, path.c_str()));
  }


  void MemoryBuffer::GetDicomQuery(const OrthancPluginWorklistQuery* query)
  {
    Clear();
    Check(OrthancPluginWorklistGetDicomQuery(GetGlobalContext(), &buffer_, query));
  }


  void OrthancString::Assign(char* str)
  {
    Clear();

    if (str != NULL)
    {
      str_ = str;
    }
  }


  void OrthancString::Clear()
  {
    if (str_ != NULL)
    {
      OrthancPluginFreeString(GetGlobalContext(), str_);
      str_ = NULL;
    }
  }


  void OrthancString::ToString(std::string& target) const
  {
    if (str_ == NULL)
    {
      target.clear();
    }
    else
    {
      target.assign(str_);
    }
  }


  void OrthancString::ToJson(Json::Value& target) const
  {
    if (str_ == NULL)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot convert an empty memory buffer to JSON");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    if (!ReadJson(target, str_))
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot convert some memory buffer to JSON");
      ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
    }
  }


  void OrthancString::ToJsonWithoutComments(Json::Value& target) const
  {
    if (str_ == NULL)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot convert an empty memory buffer to JSON");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    if (!ReadJsonWithoutComments(target, str_))
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot convert some memory buffer to JSON");
      ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
    }
  }


  void MemoryBuffer::DicomToJson(Json::Value& target,
                                 OrthancPluginDicomToJsonFormat format,
                                 OrthancPluginDicomToJsonFlags flags,
                                 uint32_t maxStringLength)
  {
    OrthancString str;
    str.Assign(OrthancPluginDicomBufferToJson
               (GetGlobalContext(), GetData(), GetSize(), format, flags, maxStringLength));
    str.ToJson(target);
  }


  bool MemoryBuffer::HttpGet(const std::string& url,
                             const std::string& username,
                             const std::string& password)
  {
    Clear();
    return CheckHttp(OrthancPluginHttpGet(GetGlobalContext(), &buffer_, url.c_str(),
                                          username.empty() ? NULL : username.c_str(),
                                          password.empty() ? NULL : password.c_str()));
  }


  bool MemoryBuffer::HttpPost(const std::string& url,
                              const std::string& body,
                              const std::string& username,
                              const std::string& password)
  {
    Clear();

    if (body.size() > 0xffffffffu)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot handle body size > 4GB");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    return CheckHttp(OrthancPluginHttpPost(GetGlobalContext(), &buffer_, url.c_str(),
                                           body.c_str(), body.size(),
                                           username.empty() ? NULL : username.c_str(),
                                           password.empty() ? NULL : password.c_str()));
  }


  bool MemoryBuffer::HttpPut(const std::string& url,
                             const std::string& body,
                             const std::string& username,
                             const std::string& password)
  {
    Clear();

    if (body.size() > 0xffffffffu)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot handle body size > 4GB");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    return CheckHttp(OrthancPluginHttpPut(GetGlobalContext(), &buffer_, url.c_str(),
                                          body.empty() ? NULL : body.c_str(),
                                          body.size(),
                                          username.empty() ? NULL : username.c_str(),
                                          password.empty() ? NULL : password.c_str()));
  }


  void MemoryBuffer::GetDicomInstance(const std::string& instanceId)
  {
    Clear();
    Check(OrthancPluginGetDicomForInstance(GetGlobalContext(), &buffer_, instanceId.c_str()));
  }


  bool HttpDelete(const std::string& url,
                  const std::string& username,
                  const std::string& password)
  {
    OrthancPluginErrorCode error = OrthancPluginHttpDelete
      (GetGlobalContext(), url.c_str(),
       username.empty() ? NULL : username.c_str(),
       password.empty() ? NULL : password.c_str());

    if (error == OrthancPluginErrorCode_Success)
    {
      return true;
    }
    else if (error == OrthancPluginErrorCode_UnknownResource ||
             error == OrthancPluginErrorCode_InexistentItem)
    {
      return false;
    }
    else
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(error);
    }
  }

  void OrthancConfiguration::LoadConfiguration()
  {
    OrthancString str;
    str.Assign(OrthancPluginGetConfiguration(GetGlobalContext()));

    if (str.GetContent() == NULL)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot access the Orthanc configuration");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    str.ToJsonWithoutComments(configuration_);

    if (configuration_.type() != Json::objectValue)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Unable to read the Orthanc configuration");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }
  }
    

  OrthancConfiguration::OrthancConfiguration()
  {
    LoadConfiguration();
  }


  OrthancConfiguration::OrthancConfiguration(bool loadConfiguration)
  {
    if (loadConfiguration)
    {
      LoadConfiguration();
    }
    else
    {
      configuration_ = Json::objectValue;
    }
  }

  OrthancConfiguration::OrthancConfiguration(const Json::Value& configuration, const std::string& path) :
    configuration_(configuration),
    path_(path)
  {
  }


  std::string OrthancConfiguration::GetPath(const std::string& key) const
  {
    if (path_.empty())
    {
      return key;
    }
    else
    {
      return path_ + "." + key;
    }
  }


  bool OrthancConfiguration::IsSection(const std::string& key) const
  {
    assert(configuration_.type() == Json::objectValue);

    return (configuration_.isMember(key) &&
            configuration_[key].type() == Json::objectValue);
  }


  void OrthancConfiguration::GetSection(OrthancConfiguration& target,
                                        const std::string& key) const
  {
    assert(configuration_.type() == Json::objectValue);

    target.path_ = GetPath(key);

    if (!configuration_.isMember(key))
    {
      target.configuration_ = Json::objectValue;
    }
    else
    {
      if (configuration_[key].type() != Json::objectValue)
      {
        ORTHANC_PLUGINS_LOG_ERROR("The configuration section \"" + target.path_ +
                                  "\" is not an associative array as expected");

        ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
      }

      target.configuration_ = configuration_[key];
    }
  }


  bool OrthancConfiguration::LookupStringValue(std::string& target,
                                               const std::string& key) const
  {
    assert(configuration_.type() == Json::objectValue);

    if (!configuration_.isMember(key))
    {
      return false;
    }

    if (configuration_[key].type() != Json::stringValue)
    {
      ORTHANC_PLUGINS_LOG_ERROR("The configuration option \"" + GetPath(key) +
                                "\" is not a string as expected");

      ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
    }

    target = configuration_[key].asString();
    return true;
  }


  bool OrthancConfiguration::LookupIntegerValue(int& target,
                                                const std::string& key) const
  {
    assert(configuration_.type() == Json::objectValue);

    if (!configuration_.isMember(key))
    {
      return false;
    }

    switch (configuration_[key].type())
    {
      case Json::intValue:
        target = configuration_[key].asInt();
        return true;

      case Json::uintValue:
        target = configuration_[key].asUInt();
        return true;

      default:
        ORTHANC_PLUGINS_LOG_ERROR("The configuration option \"" + GetPath(key) +
                                  "\" is not an integer as expected");

        ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
    }
  }


  bool OrthancConfiguration::LookupUnsignedIntegerValue(unsigned int& target,
                                                        const std::string& key) const
  {
    int tmp;
    if (!LookupIntegerValue(tmp, key))
    {
      return false;
    }

    if (tmp < 0)
    {
      ORTHANC_PLUGINS_LOG_ERROR("The configuration option \"" + GetPath(key) +
                                "\" is not a positive integer as expected");

      ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
    }
    else
    {
      target = static_cast<unsigned int>(tmp);
      return true;
    }
  }


  bool OrthancConfiguration::LookupBooleanValue(bool& target,
                                                const std::string& key) const
  {
    assert(configuration_.type() == Json::objectValue);

    if (!configuration_.isMember(key))
    {
      return false;
    }

    if (configuration_[key].type() != Json::booleanValue)
    {
      ORTHANC_PLUGINS_LOG_ERROR("The configuration option \"" + GetPath(key) +
                                "\" is not a Boolean as expected");

      ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
    }

    target = configuration_[key].asBool();
    return true;
  }


  bool OrthancConfiguration::LookupFloatValue(float& target,
                                              const std::string& key) const
  {
    assert(configuration_.type() == Json::objectValue);

    if (!configuration_.isMember(key))
    {
      return false;
    }

    switch (configuration_[key].type())
    {
      case Json::realValue:
        target = configuration_[key].asFloat();
        return true;

      case Json::intValue:
        target = static_cast<float>(configuration_[key].asInt());
        return true;

      case Json::uintValue:
        target = static_cast<float>(configuration_[key].asUInt());
        return true;

      default:
        ORTHANC_PLUGINS_LOG_ERROR("The configuration option \"" + GetPath(key) +
                                  "\" is not an integer as expected");

        ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
    }
  }


  bool OrthancConfiguration::LookupListOfStrings(std::list<std::string>& target,
                                                 const std::string& key,
                                                 bool allowSingleString) const
  {
    assert(configuration_.type() == Json::objectValue);

    target.clear();

    if (!configuration_.isMember(key))
    {
      return false;
    }

    switch (configuration_[key].type())
    {
      case Json::arrayValue:
      {
        bool ok = true;

        for (Json::Value::ArrayIndex i = 0; ok && i < configuration_[key].size(); i++)
        {
          if (configuration_[key][i].type() == Json::stringValue)
          {
            target.push_back(configuration_[key][i].asString());
          }
          else
          {
            ok = false;
          }
        }

        if (ok)
        {
          return true;
        }

        break;
      }

      case Json::stringValue:
        if (allowSingleString)
        {
          target.push_back(configuration_[key].asString());
          return true;
        }

        break;

      default:
        break;
    }

    ORTHANC_PLUGINS_LOG_ERROR("The configuration option \"" + GetPath(key) +
                              "\" is not a list of strings as expected");

    ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
  }


  bool OrthancConfiguration::LookupSetOfStrings(std::set<std::string>& target,
                                                const std::string& key,
                                                bool allowSingleString) const
  {
    std::list<std::string> lst;

    if (LookupListOfStrings(lst, key, allowSingleString))
    {
      target.clear();

      for (std::list<std::string>::const_iterator
             it = lst.begin(); it != lst.end(); ++it)
      {
        target.insert(*it);
      }

      return true;
    }
    else
    {
      return false;
    }
  }


  std::string OrthancConfiguration::GetStringValue(const std::string& key,
                                                   const std::string& defaultValue) const
  {
    std::string tmp;
    if (LookupStringValue(tmp, key))
    {
      return tmp;
    }
    else
    {
      return defaultValue;
    }
  }


  int OrthancConfiguration::GetIntegerValue(const std::string& key,
                                            int defaultValue) const
  {
    int tmp;
    if (LookupIntegerValue(tmp, key))
    {
      return tmp;
    }
    else
    {
      return defaultValue;
    }
  }


  unsigned int OrthancConfiguration::GetUnsignedIntegerValue(const std::string& key,
                                                             unsigned int defaultValue) const
  {
    unsigned int tmp;
    if (LookupUnsignedIntegerValue(tmp, key))
    {
      return tmp;
    }
    else
    {
      return defaultValue;
    }
  }


  bool OrthancConfiguration::GetBooleanValue(const std::string& key,
                                             bool defaultValue) const
  {
    bool tmp;
    if (LookupBooleanValue(tmp, key))
    {
      return tmp;
    }
    else
    {
      return defaultValue;
    }
  }


  float OrthancConfiguration::GetFloatValue(const std::string& key,
                                            float defaultValue) const
  {
    float tmp;
    if (LookupFloatValue(tmp, key))
    {
      return tmp;
    }
    else
    {
      return defaultValue;
    }
  }


  void OrthancConfiguration::GetDictionary(std::map<std::string, std::string>& target,
                                           const std::string& key) const
  {
    assert(configuration_.type() == Json::objectValue);

    target.clear();

    if (!configuration_.isMember(key))
    {
      return;
    }

    if (configuration_[key].type() != Json::objectValue)
    {
      ORTHANC_PLUGINS_LOG_ERROR("The configuration option \"" + GetPath(key) +
                                "\" is not an object as expected");

      ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
    }

    Json::Value::Members members = configuration_[key].getMemberNames();

    for (size_t i = 0; i < members.size(); i++)
    {
      const Json::Value& value = configuration_[key][members[i]];

      if (value.type() == Json::stringValue)
      {
        target[members[i]] = value.asString();
      }
      else
      {
        ORTHANC_PLUGINS_LOG_ERROR("The configuration option \"" + GetPath(key) +
                                  "\" is not a dictionary mapping strings to strings");

        ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
      }
    }
  }


  void OrthancImage::Clear()
  {
    if (image_ != NULL)
    {
      OrthancPluginFreeImage(GetGlobalContext(), image_);
      image_ = NULL;
    }
  }


  void OrthancImage::CheckImageAvailable() const
  {
    if (image_ == NULL)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Trying to access a NULL image");
      ORTHANC_PLUGINS_THROW_EXCEPTION(ParameterOutOfRange);
    }
  }


  OrthancImage::OrthancImage() :
    image_(NULL)
  {
  }


  OrthancImage::OrthancImage(OrthancPluginImage*  image) :
    image_(image)
  {
  }


  OrthancImage::OrthancImage(OrthancPluginPixelFormat  format,
                             uint32_t                  width,
                             uint32_t                  height)
  {
    image_ = OrthancPluginCreateImage(GetGlobalContext(), format, width, height);

    if (image_ == NULL)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot create an image");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }
  }


  OrthancImage::OrthancImage(OrthancPluginPixelFormat  format,
                             uint32_t                  width,
                             uint32_t                  height,
                             uint32_t                  pitch,
                             void*                     buffer)
  {
    image_ = OrthancPluginCreateImageAccessor
      (GetGlobalContext(), format, width, height, pitch, buffer);

    if (image_ == NULL)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot create an image accessor");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }
  }

  void OrthancImage::UncompressPngImage(const void* data,
                                        size_t size)
  {
    Clear();

    image_ = OrthancPluginUncompressImage(GetGlobalContext(), data, size, OrthancPluginImageFormat_Png);

    if (image_ == NULL)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot uncompress a PNG image");
      ORTHANC_PLUGINS_THROW_EXCEPTION(ParameterOutOfRange);
    }
  }


  void OrthancImage::UncompressJpegImage(const void* data,
                                         size_t size)
  {
    Clear();
    image_ = OrthancPluginUncompressImage(GetGlobalContext(), data, size, OrthancPluginImageFormat_Jpeg);
    if (image_ == NULL)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot uncompress a JPEG image");
      ORTHANC_PLUGINS_THROW_EXCEPTION(ParameterOutOfRange);
    }
  }


  void OrthancImage::DecodeDicomImage(const void* data,
                                      size_t size,
                                      unsigned int frame)
  {
    Clear();
    image_ = OrthancPluginDecodeDicomImage(GetGlobalContext(), data, size, frame);
    if (image_ == NULL)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot uncompress a DICOM image");
      ORTHANC_PLUGINS_THROW_EXCEPTION(ParameterOutOfRange);
    }
  }


  OrthancPluginPixelFormat OrthancImage::GetPixelFormat() const
  {
    CheckImageAvailable();
    return OrthancPluginGetImagePixelFormat(GetGlobalContext(), image_);
  }


  unsigned int OrthancImage::GetWidth() const
  {
    CheckImageAvailable();
    return OrthancPluginGetImageWidth(GetGlobalContext(), image_);
  }


  unsigned int OrthancImage::GetHeight() const
  {
    CheckImageAvailable();
    return OrthancPluginGetImageHeight(GetGlobalContext(), image_);
  }


  unsigned int OrthancImage::GetPitch() const
  {
    CheckImageAvailable();
    return OrthancPluginGetImagePitch(GetGlobalContext(), image_);
  }


  void* OrthancImage::GetBuffer() const
  {
    CheckImageAvailable();
    return OrthancPluginGetImageBuffer(GetGlobalContext(), image_);
  }


  void OrthancImage::CompressPngImage(MemoryBuffer& target) const
  {
    CheckImageAvailable();

    OrthancPlugins::MemoryBuffer answer;
    OrthancPluginCompressPngImage(GetGlobalContext(), *answer, GetPixelFormat(),
                                  GetWidth(), GetHeight(), GetPitch(), GetBuffer());

    target.Swap(answer);
  }


  void OrthancImage::CompressJpegImage(MemoryBuffer& target,
                                       uint8_t quality) const
  {
    CheckImageAvailable();

    OrthancPlugins::MemoryBuffer answer;
    OrthancPluginCompressJpegImage(GetGlobalContext(), *answer, GetPixelFormat(),
                                   GetWidth(), GetHeight(), GetPitch(), GetBuffer(), quality);

    target.Swap(answer);
  }


  void OrthancImage::AnswerPngImage(OrthancPluginRestOutput* output) const
  {
    CheckImageAvailable();
    OrthancPluginCompressAndAnswerPngImage(GetGlobalContext(), output, GetPixelFormat(),
                                           GetWidth(), GetHeight(), GetPitch(), GetBuffer());
  }


  void OrthancImage::AnswerJpegImage(OrthancPluginRestOutput* output,
                                     uint8_t quality) const
  {
    CheckImageAvailable();
    OrthancPluginCompressAndAnswerJpegImage(GetGlobalContext(), output, GetPixelFormat(),
                                            GetWidth(), GetHeight(), GetPitch(), GetBuffer(), quality);
  }


  OrthancPluginImage* OrthancImage::Release()
  {
    CheckImageAvailable();
    OrthancPluginImage* tmp = image_;
    image_ = NULL;
    return tmp;
  }


#if HAS_ORTHANC_PLUGIN_FIND_MATCHER == 1
  FindMatcher::FindMatcher(const OrthancPluginWorklistQuery* worklist) :
    matcher_(NULL),
    worklist_(worklist)
  {
    if (worklist_ == NULL)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(ParameterOutOfRange);
    }
  }


  void FindMatcher::SetupDicom(const void*  query,
                               uint32_t     size)
  {
    worklist_ = NULL;

    matcher_ = OrthancPluginCreateFindMatcher(GetGlobalContext(), query, size);
    if (matcher_ == NULL)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }
  }


  FindMatcher::~FindMatcher()
  {
    // The "worklist_" field

    if (matcher_ != NULL)
    {
      OrthancPluginFreeFindMatcher(GetGlobalContext(), matcher_);
    }
  }



  bool FindMatcher::IsMatch(const void*  dicom,
                            uint32_t     size) const
  {
    int32_t result;

    if (matcher_ != NULL)
    {
      result = OrthancPluginFindMatcherIsMatch(GetGlobalContext(), matcher_, dicom, size);
    }
    else if (worklist_ != NULL)
    {
      result = OrthancPluginWorklistIsMatch(GetGlobalContext(), worklist_, dicom, size);
    }
    else
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    if (result == 0)
    {
      return false;
    }
    else if (result == 1)
    {
      return true;
    }
    else
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }
  }

#endif /* HAS_ORTHANC_PLUGIN_FIND_MATCHER == 1 */

  void AnswerJson(const Json::Value& value,
                  OrthancPluginRestOutput* output)
  {
    std::string bodyString;
    WriteStyledJson(bodyString, value);    
    OrthancPluginAnswerBuffer(GetGlobalContext(), output, bodyString.c_str(), bodyString.size(), "application/json");
  }

  void AnswerString(const std::string& answer,
                    const char* mimeType,
                    OrthancPluginRestOutput* output)
  {
    OrthancPluginAnswerBuffer(GetGlobalContext(), output, answer.c_str(), answer.size(), mimeType);
  }

  void AnswerHttpError(uint16_t httpError, OrthancPluginRestOutput *output)
  {
    OrthancPluginSendHttpStatusCode(GetGlobalContext(), output, httpError);
  }

  void AnswerMethodNotAllowed(OrthancPluginRestOutput *output, const char* allowedMethods)
  {
    OrthancPluginSendMethodNotAllowed(GetGlobalContext(), output, allowedMethods);
  }

  bool RestApiGetString(std::string& result,
                        const std::string& uri,
                        bool applyPlugins)
  {
    MemoryBuffer answer;
    if (!answer.RestApiGet(uri, applyPlugins))
    {
      return false;
    }
    else
    {
      answer.ToString(result);
      return true;
    }
  }

  bool RestApiGetString(std::string& result,
                        const std::string& uri,
                        const std::map<std::string, std::string>& httpHeaders,
                        bool applyPlugins)
  {
    MemoryBuffer answer;
    if (!answer.RestApiGet(uri, httpHeaders, applyPlugins))
    {
      return false;
    }
    else
    {
      answer.ToString(result);
      return true;
    }
  }


  bool RestApiGet(Json::Value& result,
                  const std::string& uri,
                  const std::map<std::string, std::string>& httpHeaders,
                  bool applyPlugins)
  {
    MemoryBuffer answer;

    if (!answer.RestApiGet(uri, httpHeaders, applyPlugins))
    {
      return false;
    }
    else
    {
      if (!answer.IsEmpty())
      {
        answer.ToJson(result);
      }
      return true;
    }
  }


  bool RestApiGet(Json::Value& result,
                  const std::string& uri,
                  bool applyPlugins)
  {
    MemoryBuffer answer;

    if (!answer.RestApiGet(uri, applyPlugins))
    {
      return false;
    }
    else
    {
      if (!answer.IsEmpty())
      {
        answer.ToJson(result);
      }
      return true;
    }
  }


  bool RestApiPost(std::string& result,
                   const std::string& uri,
                   const void* body,
                   size_t bodySize,
                   bool applyPlugins)
  {
    MemoryBuffer answer;

    if (!answer.RestApiPost(uri, body, bodySize, applyPlugins))
    {
      return false;
    }
    else
    {
      if (!answer.IsEmpty())
      {
        result.assign(answer.GetData(), answer.GetSize());
      }
      return true;
    }
  }


  bool RestApiPost(Json::Value& result,
                   const std::string& uri,
                   const void* body,
                   size_t bodySize,
                   bool applyPlugins)
  {
    MemoryBuffer answer;

    if (!answer.RestApiPost(uri, body, bodySize, applyPlugins))
    {
      return false;
    }
    else
    {
      if (!answer.IsEmpty())
      {
        answer.ToJson(result);
      }
      return true;
    }
  }

#if HAS_ORTHANC_PLUGIN_GENERIC_CALL_REST_API == 1
  bool RestApiPost(Json::Value& result,
                   const std::string& uri,
                   const Json::Value& body,
                   const std::map<std::string, std::string>& httpHeaders,
                   bool applyPlugins)
  {
    MemoryBuffer answer;

    if (!answer.RestApiPost(uri, body, httpHeaders, applyPlugins))
    {
      return false;
    }
    else
    {
      if (!answer.IsEmpty())
      {
        answer.ToJson(result);
      }
      return true;
    }
  }
#endif


  bool RestApiPost(Json::Value& result,
                   const std::string& uri,
                   const Json::Value& body,
                   bool applyPlugins)
  {
    std::string s;
    WriteFastJson(s, body);
    return RestApiPost(result, uri, s, applyPlugins);
  }


  bool RestApiPut(Json::Value& result,
                  const std::string& uri,
                  const void* body,
                  size_t bodySize,
                  bool applyPlugins)
  {
    MemoryBuffer answer;

    if (!answer.RestApiPut(uri, body, bodySize, applyPlugins))
    {
      return false;
    }
    else
    {
      if (!answer.IsEmpty()) // i.e, on a PUT to metadata/..., orthanc returns an empty response
      {
        answer.ToJson(result);
      }
      return true;
    }
  }


  bool RestApiPut(Json::Value& result,
                  const std::string& uri,
                  const Json::Value& body,
                  bool applyPlugins)
  {
    std::string s;
    WriteFastJson(s, body);
    return RestApiPut(result, uri, s, applyPlugins);
  }


  bool RestApiDelete(const std::string& uri,
                     bool applyPlugins)
  {
    OrthancPluginErrorCode error;

    if (applyPlugins)
    {
      error = OrthancPluginRestApiDeleteAfterPlugins(GetGlobalContext(), uri.c_str());
    }
    else
    {
      error = OrthancPluginRestApiDelete(GetGlobalContext(), uri.c_str());
    }

    if (error == OrthancPluginErrorCode_Success)
    {
      return true;
    }
    else if (error == OrthancPluginErrorCode_UnknownResource ||
             error == OrthancPluginErrorCode_InexistentItem)
    {
      return false;
    }
    else
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(error);
    }
  }


  void ReportMinimalOrthancVersion(unsigned int major,
                                   unsigned int minor,
                                   unsigned int revision)
  {
    ORTHANC_PLUGINS_LOG_ERROR("Your version of the Orthanc core (" +
                              std::string(GetGlobalContext()->orthancVersion) +
                              ") is too old to run this plugin (version " +
                              boost::lexical_cast<std::string>(major) + "." +
                              boost::lexical_cast<std::string>(minor) + "." +
                              boost::lexical_cast<std::string>(revision) +
                              " is required)");
  }

  bool CheckMinimalVersion(const char* version,
                           unsigned int major,
                           unsigned int minor,
                           unsigned int revision)
  {
    if (!strcmp(version, "mainline"))
    {
      // Assume compatibility with the mainline
      return true;
    }

#ifdef _MSC_VER
#define ORTHANC_SCANF sscanf_s
#else
#define ORTHANC_SCANF sscanf
#endif

    // Parse the version
    int aa, bb, cc = 0;
    if ((ORTHANC_SCANF(version, "%4d.%4d.%4d", &aa, &bb, &cc) != 3 &&
         ORTHANC_SCANF(version, "%4d.%4d", &aa, &bb) != 2) ||
        aa < 0 ||
        bb < 0 ||
        cc < 0)
    {
      return false;
    }

    unsigned int a = static_cast<unsigned int>(aa);
    unsigned int b = static_cast<unsigned int>(bb);
    unsigned int c = static_cast<unsigned int>(cc);

    // Check the major version number

    if (a > major)
    {
      return true;
    }

    if (a < major)
    {
      return false;
    }

    // Check the minor version number
    assert(a == major);

    if (b > minor)
    {
      return true;
    }

    if (b < minor)
    {
      return false;
    }

    // Check the patch level version number
    assert(a == major && b == minor);

    if (c >= revision)
    {
      return true;
    }
    else
    {
      return false;
    }
  }


  bool CheckMinimalOrthancVersion(unsigned int major,
                                  unsigned int minor,
                                  unsigned int revision)
  {
    if (!HasGlobalContext())
    {
      ORTHANC_PLUGINS_LOG_ERROR("Bad Orthanc context in the plugin");
      return false;
    }

    return CheckMinimalVersion(GetGlobalContext()->orthancVersion,
                               major, minor, revision);
  }


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 5, 0)
  const char* AutodetectMimeType(const std::string& path)
  {
    const char* mime = OrthancPluginAutodetectMimeType(GetGlobalContext(), path.c_str());

    if (mime == NULL)
    {
      // Should never happen, just for safety
      return "application/octet-stream";
    }
    else
    {
      return mime;
    }
  }
#endif


#if HAS_ORTHANC_PLUGIN_PEERS == 1
  size_t OrthancPeers::GetPeerIndex(const std::string& name) const
  {
    size_t index;
    if (LookupName(index, name))
    {
      return index;
    }
    else
    {
      ORTHANC_PLUGINS_LOG_ERROR("Inexistent peer: " + name);
      ORTHANC_PLUGINS_THROW_EXCEPTION(UnknownResource);
    }
  }


  OrthancPeers::OrthancPeers() :
    peers_(NULL),
    timeout_(0)
  {
    peers_ = OrthancPluginGetPeers(GetGlobalContext());

    if (peers_ == NULL)
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_Plugin);
    }

    uint32_t count = OrthancPluginGetPeersCount(GetGlobalContext(), peers_);

    for (uint32_t i = 0; i < count; i++)
    {
      const char* name = OrthancPluginGetPeerName(GetGlobalContext(), peers_, i);
      if (name == NULL)
      {
        OrthancPluginFreePeers(GetGlobalContext(), peers_);
        ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_Plugin);
      }

      index_[name] = i;
    }
  }


  OrthancPeers::~OrthancPeers()
  {
    if (peers_ != NULL)
    {
      OrthancPluginFreePeers(GetGlobalContext(), peers_);
    }
  }


  bool OrthancPeers::LookupName(size_t& target,
                                const std::string& name) const
  {
    Index::const_iterator found = index_.find(name);

    if (found == index_.end())
    {
      return false;
    }
    else
    {
      target = found->second;
      return true;
    }
  }


  std::string OrthancPeers::GetPeerName(size_t index) const
  {
    if (index >= index_.size())
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_ParameterOutOfRange);
    }
    else
    {
      const char* s = OrthancPluginGetPeerName(GetGlobalContext(), peers_, static_cast<uint32_t>(index));
      if (s == NULL)
      {
        ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_Plugin);
      }
      else
      {
        return s;
      }
    }
  }


  std::string OrthancPeers::GetPeerUrl(size_t index) const
  {
    if (index >= index_.size())
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_ParameterOutOfRange);
    }
    else
    {
      const char* s = OrthancPluginGetPeerUrl(GetGlobalContext(), peers_, static_cast<uint32_t>(index));
      if (s == NULL)
      {
        ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_Plugin);
      }
      else
      {
        return s;
      }
    }
  }


  std::string OrthancPeers::GetPeerUrl(const std::string& name) const
  {
    return GetPeerUrl(GetPeerIndex(name));
  }


  bool OrthancPeers::LookupUserProperty(std::string& value,
                                        size_t index,
                                        const std::string& key) const
  {
    if (index >= index_.size())
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_ParameterOutOfRange);
    }
    else
    {
      const char* s = OrthancPluginGetPeerUserProperty(GetGlobalContext(), peers_, static_cast<uint32_t>(index), key.c_str());
      if (s == NULL)
      {
        return false;
      }
      else
      {
        value.assign(s);
        return true;
      }
    }
  }


  bool OrthancPeers::LookupUserProperty(std::string& value,
                                        const std::string& peer,
                                        const std::string& key) const
  {
    return LookupUserProperty(value, GetPeerIndex(peer), key);
  }


  bool OrthancPeers::DoGet(MemoryBuffer& target,
                           size_t index,
                           const std::string& uri,
                           const std::map<std::string, std::string>& headers) const
  {
    if (index >= index_.size())
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_ParameterOutOfRange);
    }

    OrthancPlugins::MemoryBuffer answer;
    uint16_t status;
    PluginHttpHeaders pluginHeaders(headers);

    OrthancPluginErrorCode code = OrthancPluginCallPeerApi
      (GetGlobalContext(), *answer, NULL, &status, peers_,
       static_cast<uint32_t>(index), OrthancPluginHttpMethod_Get, uri.c_str(),
       pluginHeaders.GetSize(), pluginHeaders.GetKeys(), pluginHeaders.GetValues(), NULL, 0, timeout_);

    if (code == OrthancPluginErrorCode_Success)
    {
      target.Swap(answer);
      return (status == 200);
    }
    else
    {
      return false;
    }
  }


  bool OrthancPeers::DoGet(MemoryBuffer& target,
                           const std::string& name,
                           const std::string& uri,
                           const std::map<std::string, std::string>& headers) const
  {
    size_t index;
    return (LookupName(index, name) &&
            DoGet(target, index, uri, headers));
  }


  bool OrthancPeers::DoGet(Json::Value& target,
                           size_t index,
                           const std::string& uri,
                           const std::map<std::string, std::string>& headers) const
  {
    MemoryBuffer buffer;

    if (DoGet(buffer, index, uri, headers))
    {
      buffer.ToJson(target);
      return true;
    }
    else
    {
      return false;
    }
  }


  bool OrthancPeers::DoGet(Json::Value& target,
                           const std::string& name,
                           const std::string& uri,
                           const std::map<std::string, std::string>& headers) const
  {
    MemoryBuffer buffer;

    if (DoGet(buffer, name, uri, headers))
    {
      buffer.ToJson(target);
      return true;
    }
    else
    {
      return false;
    }
  }


  bool OrthancPeers::DoPost(MemoryBuffer& target,
                            const std::string& name,
                            const std::string& uri,
                            const std::string& body,
                            const std::map<std::string, std::string>& headers) const
  {
    size_t index;
    return (LookupName(index, name) &&
            DoPost(target, index, uri, body, headers));
  }


  bool OrthancPeers::DoPost(Json::Value& target,
                            size_t index,
                            const std::string& uri,
                            const std::string& body,
                            const std::map<std::string, std::string>& headers) const
  {
    MemoryBuffer buffer;

    if (DoPost(buffer, index, uri, body, headers))
    {
      buffer.ToJson(target);
      return true;
    }
    else
    {
      return false;
    }
  }


  bool OrthancPeers::DoPost(Json::Value& target,
                            const std::string& name,
                            const std::string& uri,
                            const std::string& body,
                            const std::map<std::string, std::string>& headers) const
  {
    MemoryBuffer buffer;

    if (DoPost(buffer, name, uri, body, headers))
    {
      buffer.ToJson(target);
      return true;
    }
    else
    {
      return false;
    }
  }


  bool OrthancPeers::DoPost(MemoryBuffer& target,
                            size_t index,
                            const std::string& uri,
                            const std::string& body,
                            const std::map<std::string, std::string>& headers) const
  {
    if (index >= index_.size())
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_ParameterOutOfRange);
    }

    if (body.size() > 0xffffffffu)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot handle body size > 4GB");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    OrthancPlugins::MemoryBuffer answer;
    uint16_t status;
    PluginHttpHeaders pluginHeaders(headers);

    OrthancPluginErrorCode code = OrthancPluginCallPeerApi
      (GetGlobalContext(), *answer, NULL, &status, peers_,
       static_cast<uint32_t>(index), OrthancPluginHttpMethod_Post, uri.c_str(),
       pluginHeaders.GetSize(), pluginHeaders.GetKeys(), pluginHeaders.GetValues(), body.empty() ? NULL : body.c_str(), body.size(), timeout_);

    if (code == OrthancPluginErrorCode_Success)
    {
      target.Swap(answer);
      return (status == 200);
    }
    else
    {
      return false;
    }
  }


  bool OrthancPeers::DoPut(size_t index,
                           const std::string& uri,
                           const std::string& body,
                           const std::map<std::string, std::string>& headers) const
  {
    if (index >= index_.size())
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_ParameterOutOfRange);
    }

    if (body.size() > 0xffffffffu)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot handle body size > 4GB");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    OrthancPlugins::MemoryBuffer answer;
    uint16_t status;
    PluginHttpHeaders pluginHeaders(headers);

    OrthancPluginErrorCode code = OrthancPluginCallPeerApi
      (GetGlobalContext(), *answer, NULL, &status, peers_,
       static_cast<uint32_t>(index), OrthancPluginHttpMethod_Put, uri.c_str(),
       pluginHeaders.GetSize(), pluginHeaders.GetKeys(), pluginHeaders.GetValues(), body.empty() ? NULL : body.c_str(), body.size(), timeout_);

    if (code == OrthancPluginErrorCode_Success)
    {
      return (status == 200);
    }
    else
    {
      return false;
    }
  }


  bool OrthancPeers::DoPut(const std::string& name,
                           const std::string& uri,
                           const std::string& body,
                           const std::map<std::string, std::string>& headers) const
  {
    size_t index;
    return (LookupName(index, name) &&
            DoPut(index, uri, body, headers));
  }


  bool OrthancPeers::DoDelete(size_t index,
                              const std::string& uri,
                              const std::map<std::string, std::string>& headers) const
  {
    if (index >= index_.size())
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_ParameterOutOfRange);
    }

    OrthancPlugins::MemoryBuffer answer;
    uint16_t status;
    PluginHttpHeaders pluginHeaders(headers);

    OrthancPluginErrorCode code = OrthancPluginCallPeerApi
      (GetGlobalContext(), *answer, NULL, &status, peers_,
       static_cast<uint32_t>(index), OrthancPluginHttpMethod_Delete, uri.c_str(),
       pluginHeaders.GetSize(), pluginHeaders.GetKeys(), pluginHeaders.GetValues(), NULL, 0, timeout_);

    if (code == OrthancPluginErrorCode_Success)
    {
      return (status == 200);
    }
    else
    {
      return false;
    }
  }


  bool OrthancPeers::DoDelete(const std::string& name,
                              const std::string& uri,
                              const std::map<std::string, std::string>& headers) const
  {
    size_t index;
    return (LookupName(index, name) &&
            DoDelete(index, uri, headers));
  }
#endif





  /******************************************************************
   ** JOBS
   ******************************************************************/

#if HAS_ORTHANC_PLUGIN_JOB == 1
  void OrthancJob::CallbackFinalize(void* job)
  {
    if (job != NULL)
    {
      delete reinterpret_cast<OrthancJob*>(job);
    }
  }


  float OrthancJob::CallbackGetProgress(void* job)
  {
    assert(job != NULL);

    try
    {
      return reinterpret_cast<OrthancJob*>(job)->progress_;
    }
    catch (...)
    {
      return 0;
    }
  }


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 11, 3)
  static OrthancPluginErrorCode CopyStringToMemoryBuffer(OrthancPluginMemoryBuffer* target,
                                                         const std::string& source)
  {
    if (OrthancPluginCreateMemoryBuffer(globalContext_, target, source.size()) != OrthancPluginErrorCode_Success)
    {
      return OrthancPluginErrorCode_NotEnoughMemory;
    }
    else
    {
      if (!source.empty())
      {
        memcpy(target->data, source.c_str(), source.size());
      }
      
      return OrthancPluginErrorCode_Success;
    }
  }
#endif


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 11, 3)
  OrthancPluginErrorCode OrthancJob::CallbackGetContent(OrthancPluginMemoryBuffer* target,
                                                        void* job)
  {
    assert(job != NULL);
    OrthancJob& that = *reinterpret_cast<OrthancJob*>(job);
    return CopyStringToMemoryBuffer(target, that.content_);
  }
#else
  const char* OrthancJob::CallbackGetContent(void* job)
  {
    assert(job != NULL);

    try
    {
      return reinterpret_cast<OrthancJob*>(job)->content_.c_str();
    }
    catch (...)
    {
      return 0;
    }
  }
#endif


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 11, 3)
  int32_t OrthancJob::CallbackGetSerialized(OrthancPluginMemoryBuffer* target,
                                            void* job)
  {
    assert(job != NULL);
    OrthancJob& that = *reinterpret_cast<OrthancJob*>(job);
    
    if (that.hasSerialized_)
    {
      if (CopyStringToMemoryBuffer(target, that.serialized_) == OrthancPluginErrorCode_Success)
      {
        return 1;
      }
      else
      {
        return -1;
      }
    }
    else
    {
      return 0;
    }
  }
#else
  const char* OrthancJob::CallbackGetSerialized(void* job)
  {
    assert(job != NULL);

    try
    {
      const OrthancJob& tmp = *reinterpret_cast<OrthancJob*>(job);

      if (tmp.hasSerialized_)
      {
        return tmp.serialized_.c_str();
      }
      else
      {
        return NULL;
      }
    }
    catch (...)
    {
      return 0;
    }
  }
#endif


  OrthancPluginJobStepStatus OrthancJob::CallbackStep(void* job)
  {
    assert(job != NULL);

    try
    {
      return reinterpret_cast<OrthancJob*>(job)->Step();
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS&)
    {
      return OrthancPluginJobStepStatus_Failure;
    }
    catch (...)
    {
      return OrthancPluginJobStepStatus_Failure;
    }
  }


  OrthancPluginErrorCode OrthancJob::CallbackStop(void* job,
                                                  OrthancPluginJobStopReason reason)
  {
    assert(job != NULL);

    try
    {
      reinterpret_cast<OrthancJob*>(job)->Stop(reason);
      return OrthancPluginErrorCode_Success;
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_Plugin;
    }
  }


  OrthancPluginErrorCode OrthancJob::CallbackReset(void* job)
  {
    assert(job != NULL);

    try
    {
      reinterpret_cast<OrthancJob*>(job)->Reset();
      return OrthancPluginErrorCode_Success;
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_Plugin;
    }
  }


  void OrthancJob::ClearContent()
  {
    Json::Value empty = Json::objectValue;
    UpdateContent(empty);
  }


  void OrthancJob::UpdateContent(const Json::Value& content)
  {
    if (content.type() != Json::objectValue)
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_BadFileFormat);
    }
    else
    {
      WriteFastJson(content_, content);
    }
  }


  void OrthancJob::ClearSerialized()
  {
    hasSerialized_ = false;
    serialized_.clear();
  }


  void OrthancJob::UpdateSerialized(const Json::Value& serialized)
  {
    if (serialized.type() != Json::objectValue)
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_BadFileFormat);
    }
    else
    {
      WriteFastJson(serialized_, serialized);
      hasSerialized_ = true;
    }
  }


  void OrthancJob::UpdateProgress(float progress)
  {
    if (progress < 0 ||
        progress > 1)
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_ParameterOutOfRange);
    }

    progress_ = progress;
  }


  OrthancJob::OrthancJob(const std::string& jobType) :
    jobType_(jobType),
    progress_(0)
  {
    ClearContent();
    ClearSerialized();
  }


  OrthancPluginJob* OrthancJob::Create(OrthancJob* job)
  {
    if (job == NULL)
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_NullPointer);
    }

    OrthancPluginJob* orthanc =
#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 11, 3)
      OrthancPluginCreateJob2
#else
      OrthancPluginCreateJob
#endif
      (GetGlobalContext(), job, CallbackFinalize, job->jobType_.c_str(),
       CallbackGetProgress, CallbackGetContent, CallbackGetSerialized,
       CallbackStep, CallbackStop, CallbackReset);

    if (orthanc == NULL)
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_Plugin);
    }
    else
    {
      return orthanc;
    }
  }


  std::string OrthancJob::Submit(OrthancJob* job,
                                 int priority)
  {
    if (job == NULL)
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_NullPointer);
    }

    OrthancPluginJob* orthanc = Create(job);

    char* id = OrthancPluginSubmitJob(GetGlobalContext(), orthanc, priority);

    if (id == NULL)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Plugin cannot submit job");
      OrthancPluginFreeJob(GetGlobalContext(), orthanc);
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_Plugin);
    }
    else
    {
      std::string tmp(id);
      tmp.assign(id);
      OrthancPluginFreeString(GetGlobalContext(), id);

      return tmp;
    }
  }


  void OrthancJob::SubmitAndWait(Json::Value& result,
                                 OrthancJob* job /* takes ownership */,
                                 int priority)
  {
    std::string id = Submit(job, priority);

    for (;;)
    {
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));

      Json::Value status;
      if (!RestApiGet(status, "/jobs/" + id, false) ||
          !status.isMember("State") ||
          status["State"].type() != Json::stringValue)
      {
        ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_InexistentItem);        
      }

      const std::string state = status["State"].asString();
      if (state == "Success")
      {
        if (status.isMember("Content"))
        {
          result = status["Content"];
        }
        else
        {
          result = Json::objectValue;
        }

        return;
      }
      else if (state == "Running")
      {
        continue;
      }
      else if (!status.isMember("ErrorCode") ||
               status["ErrorCode"].type() != Json::intValue)
      {
        ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(OrthancPluginErrorCode_InternalError);
      }
      else
      {
        if (!status.isMember("ErrorDescription") ||
            status["ErrorDescription"].type() != Json::stringValue)
        {
          ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(status["ErrorCode"].asInt());
        }
        else
        {
#if HAS_ORTHANC_EXCEPTION == 1
          throw Orthanc::OrthancException(static_cast<Orthanc::ErrorCode>(status["ErrorCode"].asInt()),
                                          status["ErrorDescription"].asString());
#else
          ORTHANC_PLUGINS_LOG_ERROR("Exception while executing the job: " + status["ErrorDescription"].asString());
          ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(status["ErrorCode"].asInt());          
#endif
        }
      }
    }
  }


  void OrthancJob::SubmitFromRestApiPost(OrthancPluginRestOutput* output,
                                         const Json::Value& body,
                                         OrthancJob* job)
  {
    static const char* KEY_SYNCHRONOUS = "Synchronous";
    static const char* KEY_ASYNCHRONOUS = "Asynchronous";
    static const char* KEY_PRIORITY = "Priority";

    boost::movelib::unique_ptr<OrthancJob> protection(job);
  
    if (body.type() != Json::objectValue)
    {
#if HAS_ORTHANC_EXCEPTION == 1
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                      "Expected a JSON object in the body");
#else
      ORTHANC_PLUGINS_LOG_ERROR("Expected a JSON object in the body");
      ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
#endif
    }

    bool synchronous = true;
  
    if (body.isMember(KEY_SYNCHRONOUS))
    {
      if (body[KEY_SYNCHRONOUS].type() != Json::booleanValue)
      {
#if HAS_ORTHANC_EXCEPTION == 1
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                        "Option \"" + std::string(KEY_SYNCHRONOUS) +
                                        "\" must be Boolean");
#else
        ORTHANC_PLUGINS_LOG_ERROR("Option \"" + std::string(KEY_SYNCHRONOUS) + "\" must be Boolean");
        ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
#endif
      }
      else
      {
        synchronous = body[KEY_SYNCHRONOUS].asBool();
      }
    }

    if (body.isMember(KEY_ASYNCHRONOUS))
    {
      if (body[KEY_ASYNCHRONOUS].type() != Json::booleanValue)
      {
#if HAS_ORTHANC_EXCEPTION == 1
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                        "Option \"" + std::string(KEY_ASYNCHRONOUS) +
                                        "\" must be Boolean");
#else
        ORTHANC_PLUGINS_LOG_ERROR("Option \"" + std::string(KEY_ASYNCHRONOUS) + "\" must be Boolean");
        ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
#endif
      }
      else
      {
        synchronous = !body[KEY_ASYNCHRONOUS].asBool();
      }
    }

    int priority = 0;

    if (body.isMember(KEY_PRIORITY))
    {
      if (body[KEY_PRIORITY].type() != Json::intValue)
      {
#if HAS_ORTHANC_EXCEPTION == 1
        throw Orthanc::OrthancException(Orthanc::ErrorCode_BadFileFormat,
                                        "Option \"" + std::string(KEY_PRIORITY) +
                                        "\" must be an integer");
#else
        ORTHANC_PLUGINS_LOG_ERROR("Option \"" + std::string(KEY_PRIORITY) + "\" must be an integer");
        ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
#endif
      }
      else
      {
        priority = !body[KEY_PRIORITY].asInt();
      }
    }
  
    Json::Value result;

    if (synchronous)
    {
      OrthancPlugins::OrthancJob::SubmitAndWait(result, protection.release(), priority);
    }
    else
    {
      std::string id = OrthancPlugins::OrthancJob::Submit(protection.release(), priority);

      result = Json::objectValue;
      result["ID"] = id;
      result["Path"] = "/jobs/" + id;
    }

    std::string s = result.toStyledString();
    OrthancPluginAnswerBuffer(OrthancPlugins::GetGlobalContext(), output, s.c_str(),
                              s.size(), "application/json");
  }

#endif




  /******************************************************************
   ** METRICS
   ******************************************************************/

#if HAS_ORTHANC_PLUGIN_METRICS == 1
  MetricsTimer::MetricsTimer(const char* name) :
    name_(name)
  {
    start_ = boost::posix_time::microsec_clock::universal_time();
  }
  
  MetricsTimer::~MetricsTimer()
  {
    const boost::posix_time::ptime stop = boost::posix_time::microsec_clock::universal_time();
    const boost::posix_time::time_duration diff = stop - start_;
    OrthancPluginSetMetricsValue(GetGlobalContext(), name_.c_str(), static_cast<float>(diff.total_milliseconds()),
                                 OrthancPluginMetricsType_Timer);
  }
#endif




  /******************************************************************
   ** HTTP CLIENT
   ******************************************************************/

#if HAS_ORTHANC_PLUGIN_HTTP_CLIENT == 1
  class HttpClient::RequestBodyWrapper : public boost::noncopyable
  {
  private:
    static RequestBodyWrapper& GetObject(void* body)
    {
      assert(body != NULL);
      return *reinterpret_cast<RequestBodyWrapper*>(body);
    }

    IRequestBody&  body_;
    bool           done_;
    std::string    chunk_;

  public:
    RequestBodyWrapper(IRequestBody& body) :
      body_(body),
      done_(false)
    {
    }      

    static uint8_t IsDone(void* body)
    {
      return GetObject(body).done_;
    }
    
    static const void* GetChunkData(void* body)
    {
      return GetObject(body).chunk_.c_str();
    }
    
    static uint32_t GetChunkSize(void* body)
    {
      return static_cast<uint32_t>(GetObject(body).chunk_.size());
    }

    static OrthancPluginErrorCode Next(void* body)
    {
      RequestBodyWrapper& that = GetObject(body);
        
      if (that.done_)
      {
        return OrthancPluginErrorCode_BadSequenceOfCalls;
      }
      else
      {
        try
        {
          that.done_ = !that.body_.ReadNextChunk(that.chunk_);
          return OrthancPluginErrorCode_Success;
        }
        catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
        {
          return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
        }
        catch (...)
        {
          return OrthancPluginErrorCode_Plugin;
        }
      }
    }    
  };


#if HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_CLIENT == 1
  static OrthancPluginErrorCode AnswerAddHeaderCallback(void* answer,
                                                        const char* key,
                                                        const char* value)
  {
    assert(answer != NULL && key != NULL && value != NULL);

    try
    {
      reinterpret_cast<HttpClient::IAnswer*>(answer)->AddHeader(key, value);
      return OrthancPluginErrorCode_Success;
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_Plugin;
    }
  }
#endif


#if HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_CLIENT == 1
  static OrthancPluginErrorCode AnswerAddChunkCallback(void* answer,
                                                       const void* data,
                                                       uint32_t size)
  {
    assert(answer != NULL);

    try
    {
      reinterpret_cast<HttpClient::IAnswer*>(answer)->AddChunk(data, size);
      return OrthancPluginErrorCode_Success;
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_Plugin;
    }
  }
#endif


  HttpClient::HttpClient() :
    httpStatus_(0),
    method_(OrthancPluginHttpMethod_Get),
    timeout_(0),
    pkcs11_(false),
    chunkedBody_(NULL),
    allowChunkedTransfers_(true)
  {
  }


  void HttpClient::AddHeaders(const HttpHeaders& headers)
  {
    for (HttpHeaders::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
      headers_[it->first] = it->second;
    }
  }

  
  void HttpClient::SetCredentials(const std::string& username,
                                  const std::string& password)
  {
    username_ = username;
    password_ = password;
  }

  
  void HttpClient::ClearCredentials()
  {
    username_.clear();
    password_.clear();
  }


  void HttpClient::SetCertificate(const std::string& certificateFile,
                                  const std::string& keyFile,
                                  const std::string& keyPassword)
  {
    certificateFile_ = certificateFile;
    certificateKeyFile_ = keyFile;
    certificateKeyPassword_ = keyPassword;
  }

  
  void HttpClient::ClearCertificate()
  {
    certificateFile_.clear();
    certificateKeyFile_.clear();
    certificateKeyPassword_.clear();
  }


  void HttpClient::ClearBody()
  {
    fullBody_.clear();
    chunkedBody_ = NULL;
  }

  
  void HttpClient::SwapBody(std::string& body)
  {
    fullBody_.swap(body);
    chunkedBody_ = NULL;
  }

  
  void HttpClient::SetBody(const std::string& body)
  {
    fullBody_ = body;
    chunkedBody_ = NULL;
  }

  
  void HttpClient::SetBody(IRequestBody& body)
  {
    fullBody_.clear();
    chunkedBody_ = &body;
  }


  namespace
  {
    class HeadersWrapper : public boost::noncopyable
    {
    private:
      std::vector<const char*>  headersKeys_;
      std::vector<const char*>  headersValues_;

    public:
      HeadersWrapper(const HttpClient::HttpHeaders& headers)
      {
        headersKeys_.reserve(headers.size());
        headersValues_.reserve(headers.size());

        for (HttpClient::HttpHeaders::const_iterator it = headers.begin(); it != headers.end(); ++it)
        {
          headersKeys_.push_back(it->first.c_str());
          headersValues_.push_back(it->second.c_str());
        }
      }

      void AddStaticString(const char* key,
                           const char* value)
      {
        headersKeys_.push_back(key);
        headersValues_.push_back(value);
      }

      uint32_t GetCount() const
      {
        return headersKeys_.size();
      }

      const char* const* GetKeys() const
      {
        return headersKeys_.empty() ? NULL : &headersKeys_[0];
      }

      const char* const* GetValues() const
      {
        return headersValues_.empty() ? NULL : &headersValues_[0];
      }
    };


    class MemoryRequestBody : public HttpClient::IRequestBody
    {
    private:
      std::string  body_;
      bool         done_;

    public:
      MemoryRequestBody(const std::string& body) :
        body_(body),
        done_(false)
      {
        if (body_.empty())
        {
          done_ = true;
        }
      }

      virtual bool ReadNextChunk(std::string& chunk)
      {
        if (done_)
        {
          return false;
        }
        else
        {
          chunk.swap(body_);
          done_ = true;
          return true;
        }
      }
    };


    // This class mimics Orthanc::ChunkedBuffer
    class ChunkedBuffer : public boost::noncopyable
    {
    private:
      typedef std::list<std::string*>  Content;

      Content  content_;
      size_t   size_;

    public:
      ChunkedBuffer() :
        size_(0)
      {
      }

      ~ChunkedBuffer()
      {
        Clear();
      }

      void Clear()
      {
        for (Content::iterator it = content_.begin(); it != content_.end(); ++it)
        {
          assert(*it != NULL);
          delete *it;
        }

        size_ = 0;
        content_.clear();
      }

      /**
       * Since Orthanc 1.9.3, this function also clears the content of
       * the ChunkedBuffer in order to mimic the behavior of the
       * original class "Orthanc::ChunkedBuffer". This prevents the
       * forgetting of calling "Clear()" in order to reduce memory
       * consumption.
       **/
      void Flatten(std::string& target)
      {
        target.resize(size_);

        size_t pos = 0;

        for (Content::const_iterator it = content_.begin(); it != content_.end(); ++it)
        {
          assert(*it != NULL);
          size_t s = (*it)->size();

          if (s != 0)
          {
            memcpy(&target[pos], (*it)->c_str(), s);
            pos += s;
          }

          delete *it;
        }

        assert(pos == target.size());

        size_ = 0;
        content_.clear();
      }

      void AddChunk(const void* data,
                    size_t size)
      {
        content_.push_back(new std::string(reinterpret_cast<const char*>(data), size));
        size_ += size;
      }

      void AddChunk(const std::string& chunk)
      {
        content_.push_back(new std::string(chunk));
        size_ += chunk.size();
      }
    };


#if HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_CLIENT == 1
    class MemoryAnswer : public HttpClient::IAnswer
    {
    private:
      HttpClient::HttpHeaders  headers_;
      ChunkedBuffer            body_;

    public:
      const HttpClient::HttpHeaders& GetHeaders() const
      {
        return headers_;
      }

      ChunkedBuffer& GetBody()
      {
        return body_;
      }

      virtual void AddHeader(const std::string& key,
                             const std::string& value)
      {
        headers_[key] = value;
      }

      virtual void AddChunk(const void* data,
                            size_t size)
      {
        body_.AddChunk(data, size);
      }
    };
#endif
  }


#if HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_CLIENT == 1
  void HttpClient::ExecuteWithStream(uint16_t& httpStatus,
                                     IAnswer& answer,
                                     IRequestBody& body) const
  {
    HeadersWrapper h(headers_);

    if (method_ == OrthancPluginHttpMethod_Post ||
        method_ == OrthancPluginHttpMethod_Put)
    {
      // Automatically set the "Transfer-Encoding" header if absent
      bool found = false;

      for (HttpHeaders::const_iterator it = headers_.begin(); it != headers_.end(); ++it)
      {
        if (boost::iequals(it->first, "Transfer-Encoding"))
        {
          found = true;
          break;
        }
      }

      if (!found)
      {
        h.AddStaticString("Transfer-Encoding", "chunked");
      }
    }

    RequestBodyWrapper request(body);
        
    OrthancPluginErrorCode error = OrthancPluginChunkedHttpClient(
      GetGlobalContext(),
      &answer,
      AnswerAddChunkCallback,
      AnswerAddHeaderCallback,
      &httpStatus,
      method_,
      url_.c_str(),
      h.GetCount(),
      h.GetKeys(),
      h.GetValues(),
      &request,
      RequestBodyWrapper::IsDone,
      RequestBodyWrapper::GetChunkData,
      RequestBodyWrapper::GetChunkSize,
      RequestBodyWrapper::Next,
      username_.empty() ? NULL : username_.c_str(),
      password_.empty() ? NULL : password_.c_str(),
      timeout_,
      certificateFile_.empty() ? NULL : certificateFile_.c_str(),
      certificateFile_.empty() ? NULL : certificateKeyFile_.c_str(),
      certificateFile_.empty() ? NULL : certificateKeyPassword_.c_str(),
      pkcs11_ ? 1 : 0);

    if (error != OrthancPluginErrorCode_Success)
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(error);
    }
  }
#endif    


  void HttpClient::ExecuteWithoutStream(uint16_t& httpStatus,
                                        HttpHeaders& answerHeaders,
                                        std::string& answerBody,
                                        const std::string& body) const
  {
    HeadersWrapper headers(headers_);

    MemoryBuffer answerBodyBuffer, answerHeadersBuffer;

    if (body.size() > 0xffffffffu)
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot handle body size > 4GB");
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    OrthancPluginErrorCode error = OrthancPluginHttpClient(
      GetGlobalContext(),
      *answerBodyBuffer,
      *answerHeadersBuffer,
      &httpStatus,
      method_,
      url_.c_str(),
      headers.GetCount(),
      headers.GetKeys(),
      headers.GetValues(),
      body.empty() ? NULL : body.c_str(),
      body.size(),
      username_.empty() ? NULL : username_.c_str(),
      password_.empty() ? NULL : password_.c_str(),
      timeout_,
      certificateFile_.empty() ? NULL : certificateFile_.c_str(),
      certificateFile_.empty() ? NULL : certificateKeyFile_.c_str(),
      certificateFile_.empty() ? NULL : certificateKeyPassword_.c_str(),
      pkcs11_ ? 1 : 0);

    if (error != OrthancPluginErrorCode_Success)
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(error);
    }

    Json::Value v;
    answerHeadersBuffer.ToJson(v);

    if (v.type() != Json::objectValue)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
    }

    Json::Value::Members members = v.getMemberNames();
    answerHeaders.clear();

    for (size_t i = 0; i < members.size(); i++)
    {
      const Json::Value& h = v[members[i]];
      if (h.type() != Json::stringValue)
      {
        ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
      }
      else
      {
        answerHeaders[members[i]] = h.asString();
      }
    }

    answerBodyBuffer.ToString(answerBody);
  }


  void HttpClient::Execute(IAnswer& answer)
  {
#if HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_CLIENT == 1
    if (allowChunkedTransfers_)
    {
      if (chunkedBody_ != NULL)
      {
        ExecuteWithStream(httpStatus_, answer, *chunkedBody_);
      }
      else
      {
        MemoryRequestBody wrapper(fullBody_);
        ExecuteWithStream(httpStatus_, answer, wrapper);
      }

      return;
    }
#endif
    
    // Compatibility mode for Orthanc SDK <= 1.5.6 or if chunked
    // transfers are disabled. This results in higher memory usage
    // (all chunks from the answer body are sent at once)

    HttpHeaders answerHeaders;
    std::string answerBody;
    Execute(answerHeaders, answerBody);

    for (HttpHeaders::const_iterator it = answerHeaders.begin(); 
         it != answerHeaders.end(); ++it)
    {
      answer.AddHeader(it->first, it->second);      
    }

    if (!answerBody.empty())
    {
      answer.AddChunk(answerBody.c_str(), answerBody.size());
    }
  }


  void HttpClient::Execute(HttpHeaders& answerHeaders /* out */,
                           std::string& answerBody /* out */)
  {
#if HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_CLIENT == 1
    if (allowChunkedTransfers_)
    {
      MemoryAnswer answer;
      Execute(answer);
      answerHeaders = answer.GetHeaders();
      answer.GetBody().Flatten(answerBody);
      return;
    }
#endif
    
    // Compatibility mode for Orthanc SDK <= 1.5.6 or if chunked
    // transfers are disabled. This results in higher memory usage
    // (all chunks from the request body are sent at once)

    if (chunkedBody_ != NULL)
    {
      ChunkedBuffer buffer;
      
      std::string chunk;
      while (chunkedBody_->ReadNextChunk(chunk))
      {
        buffer.AddChunk(chunk);
      }

      std::string body;
      buffer.Flatten(body);

      ExecuteWithoutStream(httpStatus_, answerHeaders, answerBody, body);
    }
    else
    {
      ExecuteWithoutStream(httpStatus_, answerHeaders, answerBody, fullBody_);
    }
  }


  void HttpClient::Execute(HttpHeaders& answerHeaders /* out */,
                           Json::Value& answerBody /* out */)
  {
    std::string body;
    Execute(answerHeaders, body);
    
    if (!ReadJson(answerBody, body))
    {
      ORTHANC_PLUGINS_LOG_ERROR("Cannot convert HTTP answer body to JSON");
      ORTHANC_PLUGINS_THROW_EXCEPTION(BadFileFormat);
    }
  }


  void HttpClient::Execute()
  {
    HttpHeaders answerHeaders;
    std::string body;
    Execute(answerHeaders, body);
  }

#endif  /* HAS_ORTHANC_PLUGIN_HTTP_CLIENT == 1 */





  /******************************************************************
   ** CHUNKED HTTP SERVER
   ******************************************************************/

  namespace Internals
  {
    void NullRestCallback(OrthancPluginRestOutput* output,
                          const char* url,
                          const OrthancPluginHttpRequest* request)
    {
    }
  
    IChunkedRequestReader *NullChunkedRestCallback(const char* url,
                                                   const OrthancPluginHttpRequest* request)
    {
      return NULL;
    }


#if HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_SERVER == 1

    OrthancPluginErrorCode ChunkedRequestReaderAddChunk(
      OrthancPluginServerChunkedRequestReader* reader,
      const void*                              data,
      uint32_t                                 size)
    {
      try
      {
        if (reader == NULL)
        {
          return OrthancPluginErrorCode_InternalError;
        }

        reinterpret_cast<IChunkedRequestReader*>(reader)->AddChunk(data, size);
        return OrthancPluginErrorCode_Success;
      }
      catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
      {
        return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
      }
      catch (boost::bad_lexical_cast&)
      {
        return OrthancPluginErrorCode_BadFileFormat;
      }
      catch (...)
      {
        return OrthancPluginErrorCode_Plugin;
      }
    }

    
    OrthancPluginErrorCode ChunkedRequestReaderExecute(
      OrthancPluginServerChunkedRequestReader* reader,
      OrthancPluginRestOutput*                 output)
    {
      try
      {
        if (reader == NULL)
        {
          return OrthancPluginErrorCode_InternalError;
        }

        reinterpret_cast<IChunkedRequestReader*>(reader)->Execute(output);
        return OrthancPluginErrorCode_Success;
      }
      catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
      {
        return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
      }
      catch (boost::bad_lexical_cast&)
      {
        return OrthancPluginErrorCode_BadFileFormat;
      }
      catch (...)
      {
        return OrthancPluginErrorCode_Plugin;
      }
    }

    
    void ChunkedRequestReaderFinalize(
      OrthancPluginServerChunkedRequestReader* reader)
    {
      if (reader != NULL)
      {
        delete reinterpret_cast<IChunkedRequestReader*>(reader);
      }
    }

#else
    
    OrthancPluginErrorCode ChunkedRestCompatibility(OrthancPluginRestOutput* output,
                                                    const char* url,
                                                    const OrthancPluginHttpRequest* request,
                                                    RestCallback         GetHandler,
                                                    ChunkedRestCallback  PostHandler,
                                                    RestCallback         DeleteHandler,
                                                    ChunkedRestCallback  PutHandler)
    {
      try
      {
        std::string allowed;

        if (GetHandler != Internals::NullRestCallback)
        {
          allowed += "GET";
        }

        if (PostHandler != Internals::NullChunkedRestCallback)
        {
          if (!allowed.empty())
          {
            allowed += ",";
          }
        
          allowed += "POST";
        }

        if (DeleteHandler != Internals::NullRestCallback)
        {
          if (!allowed.empty())
          {
            allowed += ",";
          }
        
          allowed += "DELETE";
        }

        if (PutHandler != Internals::NullChunkedRestCallback)
        {
          if (!allowed.empty())
          {
            allowed += ",";
          }
        
          allowed += "PUT";
        }
      
        switch (request->method)
        {
          case OrthancPluginHttpMethod_Get:
            if (GetHandler == Internals::NullRestCallback)
            {
              OrthancPluginSendMethodNotAllowed(GetGlobalContext(), output, allowed.c_str());
            }
            else
            {
              GetHandler(output, url, request);
            }

            break;

          case OrthancPluginHttpMethod_Post:
            if (PostHandler == Internals::NullChunkedRestCallback)
            {
              OrthancPluginSendMethodNotAllowed(GetGlobalContext(), output, allowed.c_str());
            }
            else
            {
              boost::movelib::unique_ptr<IChunkedRequestReader> reader(PostHandler(url, request));
              if (reader.get() == NULL)
              {
                ORTHANC_PLUGINS_THROW_EXCEPTION(Plugin);
              }
              else
              {
                reader->AddChunk(request->body, request->bodySize);
                reader->Execute(output);
              }
            }

            break;

          case OrthancPluginHttpMethod_Delete:
            if (DeleteHandler == Internals::NullRestCallback)
            {
              OrthancPluginSendMethodNotAllowed(GetGlobalContext(), output, allowed.c_str());
            }
            else
            {
              DeleteHandler(output, url, request);
            }

            break;

          case OrthancPluginHttpMethod_Put:
            if (PutHandler == Internals::NullChunkedRestCallback)
            {
              OrthancPluginSendMethodNotAllowed(GetGlobalContext(), output, allowed.c_str());
            }
            else
            {
              boost::movelib::unique_ptr<IChunkedRequestReader> reader(PutHandler(url, request));
              if (reader.get() == NULL)
              {
                ORTHANC_PLUGINS_THROW_EXCEPTION(Plugin);
              }
              else
              {
                reader->AddChunk(request->body, request->bodySize);
                reader->Execute(output);
              }
            }

            break;

          default:
            ORTHANC_PLUGINS_THROW_EXCEPTION(InternalError);
        }

        return OrthancPluginErrorCode_Success;
      }
      catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
      {
#if HAS_ORTHANC_EXCEPTION == 1 && HAS_ORTHANC_PLUGIN_EXCEPTION_DETAILS == 1
        if (HasGlobalContext() &&
            e.HasDetails())
        {
          // The "false" instructs Orthanc not to log the detailed
          // error message. This is to avoid duplicating the details,
          // because "OrthancException" already does it on construction.
          OrthancPluginSetHttpErrorDetails
            (GetGlobalContext(), output, e.GetDetails(), false);
        }
#endif

        return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
      }
      catch (boost::bad_lexical_cast&)
      {
        return OrthancPluginErrorCode_BadFileFormat;
      }
      catch (...)
      {
        return OrthancPluginErrorCode_Plugin;
      }
    }
#endif
  }


#if HAS_ORTHANC_PLUGIN_STORAGE_COMMITMENT_SCP == 1
  OrthancPluginErrorCode IStorageCommitmentScpHandler::Lookup(
    OrthancPluginStorageCommitmentFailureReason* target,
    void* rawHandler,
    const char* sopClassUid,
    const char* sopInstanceUid)
  {
    assert(target != NULL &&
           rawHandler != NULL);
      
    try
    {
      IStorageCommitmentScpHandler& handler = *reinterpret_cast<IStorageCommitmentScpHandler*>(rawHandler);
      *target = handler.Lookup(sopClassUid, sopInstanceUid);
      return OrthancPluginErrorCode_Success;
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_Plugin;
    }
  }
#endif


#if HAS_ORTHANC_PLUGIN_STORAGE_COMMITMENT_SCP == 1
  void IStorageCommitmentScpHandler::Destructor(void* rawHandler)
  {
    assert(rawHandler != NULL);
    delete reinterpret_cast<IStorageCommitmentScpHandler*>(rawHandler);
  }
#endif


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 6, 1)    
  DicomInstance::DicomInstance(const OrthancPluginDicomInstance* instance) :
    toFree_(false),
    instance_(instance)
  {
  }
#else
  DicomInstance::DicomInstance(OrthancPluginDicomInstance* instance) :
    toFree_(false),
    instance_(instance)
  {
  }
#endif


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
  DicomInstance::DicomInstance(const void* buffer,
                               size_t size) :
    toFree_(true),
    instance_(OrthancPluginCreateDicomInstance(GetGlobalContext(), buffer, size))
  {
    if (instance_ == NULL)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(NullPointer);
    }
  }
#endif


  DicomInstance::~DicomInstance()
  {
#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
    if (toFree_ &&
        instance_ != NULL)
    {
      OrthancPluginFreeDicomInstance(
        GetGlobalContext(), const_cast<OrthancPluginDicomInstance*>(instance_));
    }
#endif
  }

  
  std::string DicomInstance::GetRemoteAet() const
  {
    const char* s = OrthancPluginGetInstanceRemoteAet(GetGlobalContext(), instance_);
    if (s == NULL)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(Plugin);
    }
    else
    {
      return std::string(s);
    }
  }


  void DicomInstance::GetJson(Json::Value& target) const
  {
    OrthancString s;
    s.Assign(OrthancPluginGetInstanceJson(GetGlobalContext(), instance_));
    s.ToJson(target);
  }
  

  void DicomInstance::GetSimplifiedJson(Json::Value& target) const
  {
    OrthancString s;
    s.Assign(OrthancPluginGetInstanceSimplifiedJson(GetGlobalContext(), instance_));
    s.ToJson(target);
  }


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 6, 1)
  std::string DicomInstance::GetTransferSyntaxUid() const
  {
    OrthancString s;
    s.Assign(OrthancPluginGetInstanceTransferSyntaxUid(GetGlobalContext(), instance_));

    std::string result;
    s.ToString(result);
    return result;
  }
#endif

  
#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 6, 1)
  bool DicomInstance::HasPixelData() const
  {
    int32_t result = OrthancPluginHasInstancePixelData(GetGlobalContext(), instance_);
    if (result < 0)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(Plugin);
    }
    else
    {
      return (result != 0);
    }
  }
#endif


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)  
  void DicomInstance::GetRawFrame(std::string& target,
                                  unsigned int frameIndex) const
  {
    MemoryBuffer buffer;
    OrthancPluginErrorCode code = OrthancPluginGetInstanceRawFrame(
      GetGlobalContext(), *buffer, instance_, frameIndex);

    if (code == OrthancPluginErrorCode_Success)
    {
      buffer.ToString(target);
    }
    else
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(code);
    }
  }
#endif


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)  
  OrthancImage* DicomInstance::GetDecodedFrame(unsigned int frameIndex) const
  {
    OrthancPluginImage* image = OrthancPluginGetInstanceDecodedFrame(
      GetGlobalContext(), instance_, frameIndex);

    if (image == NULL)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(Plugin);
    }
    else
    {
      return new OrthancImage(image);
    }
  }
#endif  


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
  void DicomInstance::Serialize(std::string& target) const
  {
    MemoryBuffer buffer;
    OrthancPluginErrorCode code = OrthancPluginSerializeDicomInstance(
      GetGlobalContext(), *buffer, instance_);

    if (code == OrthancPluginErrorCode_Success)
    {
      buffer.ToString(target);
    }
    else
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(code);
    }
  }
#endif
  

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
  DicomInstance* DicomInstance::Transcode(const void* buffer,
                                          size_t size,
                                          const std::string& transferSyntax)
  {
    OrthancPluginDicomInstance* instance = OrthancPluginTranscodeDicomInstance(
      GetGlobalContext(), buffer, size, transferSyntax.c_str());

    if (instance == NULL)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(Plugin);
    }
    else
    {
      boost::movelib::unique_ptr<DicomInstance> result(new DicomInstance(instance));
      result->toFree_ = true;
      return result.release();
    }
  }
#endif


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 12, 1)
  DicomInstance* DicomInstance::Load(const std::string& instanceId,
                                     OrthancPluginLoadDicomInstanceMode mode)
  {
    OrthancPluginDicomInstance* instance = OrthancPluginLoadDicomInstance(
      GetGlobalContext(), instanceId.c_str(), mode);

    if (instance == NULL)
    {
      ORTHANC_PLUGINS_THROW_EXCEPTION(Plugin);
    }
    else
    {
      boost::movelib::unique_ptr<DicomInstance> result(new DicomInstance(instance));
      result->toFree_ = true;
      return result.release();
    }
  }
#endif


#if HAS_ORTHANC_PLUGIN_WEBDAV == 1
  static std::vector<std::string> WebDavConvertPath(uint32_t pathSize,
                                                    const char* const*  pathItems)
  {
    std::vector<std::string> result(pathSize);

    for (uint32_t i = 0; i < pathSize; i++)
    {
      result[i] = pathItems[i];
    }

    return result;
  }
#endif
  
    
#if HAS_ORTHANC_PLUGIN_WEBDAV == 1
  static OrthancPluginErrorCode WebDavIsExistingFolder(uint8_t*            isExisting,
                                                       uint32_t            pathSize,
                                                       const char* const*  pathItems,
                                                       void*               payload)
  {
    IWebDavCollection& that = *reinterpret_cast<IWebDavCollection*>(payload);

    try
    {
      *isExisting = (that.IsExistingFolder(WebDavConvertPath(pathSize, pathItems)) ? 1 : 0);
      return OrthancPluginErrorCode_Success;
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_Plugin;
    }
  }
#endif

  
#if HAS_ORTHANC_PLUGIN_WEBDAV == 1
  static OrthancPluginErrorCode WebDavListFolder(uint8_t*                        isExisting,
                                                 OrthancPluginWebDavCollection*  collection,
                                                 OrthancPluginWebDavAddFile      addFile,
                                                 OrthancPluginWebDavAddFolder    addFolder,
                                                 uint32_t                        pathSize,
                                                 const char* const*              pathItems,
                                                 void*                           payload)
  {
    IWebDavCollection& that = *reinterpret_cast<IWebDavCollection*>(payload);
      
    try
    {
      std::list<IWebDavCollection::FileInfo> files;
      std::list<IWebDavCollection::FolderInfo> subfolders;
      
      if (!that.ListFolder(files, subfolders, WebDavConvertPath(pathSize, pathItems)))
      {
        *isExisting = 0;
      }
      else
      {
        *isExisting = 1;
      
        for (std::list<IWebDavCollection::FileInfo>::const_iterator
               it = files.begin(); it != files.end(); ++it)
        {
          OrthancPluginErrorCode code = addFile(
            collection, it->GetName().c_str(), it->GetContentSize(),
            it->GetMimeType().c_str(), it->GetDateTime().c_str());
        
          if (code != OrthancPluginErrorCode_Success)
          {
            return code;
          }
        }
      
        for (std::list<IWebDavCollection::FolderInfo>::const_iterator it =
               subfolders.begin(); it != subfolders.end(); ++it)
        {
          OrthancPluginErrorCode code = addFolder(
            collection, it->GetName().c_str(), it->GetDateTime().c_str());
        
          if (code != OrthancPluginErrorCode_Success)
          {
            return code;
          }
        }
      }
      
      return OrthancPluginErrorCode_Success;
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_Plugin;
    }
  }
#endif    


#if HAS_ORTHANC_PLUGIN_WEBDAV == 1
  static OrthancPluginErrorCode WebDavRetrieveFile(OrthancPluginWebDavCollection*   collection,
                                                   OrthancPluginWebDavRetrieveFile  retrieveFile,
                                                   uint32_t                         pathSize,
                                                   const char* const*               pathItems,
                                                   void*                            payload)
  {
    IWebDavCollection& that = *reinterpret_cast<IWebDavCollection*>(payload);

    try
    {
      std::string content, mime, dateTime;
        
      if (that.GetFile(content, mime, dateTime, WebDavConvertPath(pathSize, pathItems)))
      {
        return retrieveFile(collection, content.empty() ? NULL : content.c_str(),
                            content.size(), mime.c_str(), dateTime.c_str());
      }
      else
      {
        // Inexisting file
        return OrthancPluginErrorCode_Success;
      }
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_InternalError;
    }
  }  
#endif


#if HAS_ORTHANC_PLUGIN_WEBDAV == 1
  static OrthancPluginErrorCode WebDavStoreFileCallback(uint8_t*            isReadOnly, /* out */
                                                        uint32_t            pathSize,
                                                        const char* const*  pathItems,
                                                        const void*         data,
                                                        uint64_t            size,
                                                        void*               payload)
  {
    IWebDavCollection& that = *reinterpret_cast<IWebDavCollection*>(payload);

    try
    {
      if (static_cast<uint64_t>(static_cast<size_t>(size)) != size)
      {
        ORTHANC_PLUGINS_THROW_EXCEPTION(NotEnoughMemory);
      }
      
      *isReadOnly = (that.StoreFile(WebDavConvertPath(pathSize, pathItems), data,
                                    static_cast<size_t>(size)) ? 1 : 0);
      return OrthancPluginErrorCode_Success;
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_InternalError;
    }
  }
#endif

  
#if HAS_ORTHANC_PLUGIN_WEBDAV == 1
  static OrthancPluginErrorCode WebDavCreateFolderCallback(uint8_t*            isReadOnly, /* out */
                                                           uint32_t            pathSize,
                                                           const char* const*  pathItems,
                                                           void*               payload)
  {
    IWebDavCollection& that = *reinterpret_cast<IWebDavCollection*>(payload);

    try
    {
      *isReadOnly = (that.CreateFolder(WebDavConvertPath(pathSize, pathItems)) ? 1 : 0);
      return OrthancPluginErrorCode_Success;
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_InternalError;
    }
  }
#endif
  
  
#if HAS_ORTHANC_PLUGIN_WEBDAV == 1
  static OrthancPluginErrorCode WebDavDeleteItemCallback(uint8_t*            isReadOnly, /* out */
                                                         uint32_t            pathSize,
                                                         const char* const*  pathItems,
                                                         void*               payload)
  {
    IWebDavCollection& that = *reinterpret_cast<IWebDavCollection*>(payload);

    try
    {
      *isReadOnly = (that.DeleteItem(WebDavConvertPath(pathSize, pathItems)) ? 1 : 0);
      return OrthancPluginErrorCode_Success;
    }
    catch (ORTHANC_PLUGINS_EXCEPTION_CLASS& e)
    {
      return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
    }
    catch (...)
    {
      return OrthancPluginErrorCode_InternalError;
    }
  }
#endif

  
#if HAS_ORTHANC_PLUGIN_WEBDAV == 1
  void IWebDavCollection::Register(const std::string& uri,
                                   IWebDavCollection& collection)
  {
    OrthancPluginErrorCode code = OrthancPluginRegisterWebDavCollection(
      GetGlobalContext(), uri.c_str(), WebDavIsExistingFolder, WebDavListFolder, WebDavRetrieveFile,
      WebDavStoreFileCallback, WebDavCreateFolderCallback, WebDavDeleteItemCallback, &collection);

    if (code != OrthancPluginErrorCode_Success)
    {
      ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(code);
    }
  }
#endif

  void GetHttpHeaders(std::map<std::string, std::string>& result, const OrthancPluginHttpRequest* request)
  {
    result.clear();

    for (uint32_t i = 0; i < request->headersCount; ++i)
    {
      result[request->headersKeys[i]] = request->headersValues[i];
    }    
  }

#if !ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 12, 4)
  static void SetPluginProperty(const std::string& pluginIdentifier,
                                _OrthancPluginProperty property,
                                const std::string& value)
  {
    _OrthancPluginSetPluginProperty params;
    params.plugin = pluginIdentifier.c_str();
    params.property = property;
    params.value = value.c_str();

    GetGlobalContext()->InvokeService(GetGlobalContext(), _OrthancPluginService_SetPluginProperty, &params);
  }
#endif

  void SetRootUri(const std::string& pluginIdentifier,
                  const std::string& uri)
  {
#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 12, 4)
    OrthancPluginSetRootUri2(GetGlobalContext(), pluginIdentifier.c_str(), uri.c_str());
#else
    SetPluginProperty(pluginIdentifier, _OrthancPluginProperty_RootUri, uri);
#endif
  }

  void SetDescription(const std::string& pluginIdentifier,
                      const std::string& description)
  {
#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 12, 4)
    OrthancPluginSetDescription2(GetGlobalContext(), pluginIdentifier.c_str(), description.c_str());
#else
    SetPluginProperty(pluginIdentifier, _OrthancPluginProperty_Description, description);
#endif
  }

  void ExtendOrthancExplorer(const std::string& pluginIdentifier,
                             const std::string& javascript)
  {
#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 12, 4)
    OrthancPluginExtendOrthancExplorer2(GetGlobalContext(), pluginIdentifier.c_str(), javascript.c_str());
#else
    SetPluginProperty(pluginIdentifier, _OrthancPluginProperty_OrthancExplorer, javascript);
#endif
  }
}
