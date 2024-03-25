/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2024 Osimis S.A., Belgium
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


#pragma once

#include "OrthancPluginException.h"

#include <orthanc/OrthancCPlugin.h>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <json/value.h>
#include <vector>
#include <list>
#include <set>
#include <map>



/**
 * The definition of ORTHANC_PLUGINS_VERSION_IS_ABOVE below is for
 * backward compatibility with Orthanc SDK <= 1.3.0.
 * 
 *   $ hg diff -r Orthanc-1.3.0:Orthanc-1.3.1 ../../../Plugins/Include/orthanc/OrthancCPlugin.h
 *
 **/
#if !defined(ORTHANC_PLUGINS_VERSION_IS_ABOVE)
#define ORTHANC_PLUGINS_VERSION_IS_ABOVE(major, minor, revision)        \
  (ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER > major ||                      \
   (ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER == major &&                    \
    (ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER > minor ||                    \
     (ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER == minor &&                  \
      ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER >= revision))))
#endif


#if !defined(ORTHANC_FRAMEWORK_VERSION_IS_ABOVE)
#define ORTHANC_FRAMEWORK_VERSION_IS_ABOVE(major, minor, revision)      \
  (ORTHANC_VERSION_MAJOR > major ||                                     \
   (ORTHANC_VERSION_MAJOR == major &&                                   \
    (ORTHANC_VERSION_MINOR > minor ||                                   \
     (ORTHANC_VERSION_MINOR == minor &&                                 \
      ORTHANC_VERSION_REVISION >= revision))))
#endif


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 2, 0)
// The "OrthancPluginFindMatcher()" primitive was introduced in Orthanc 1.2.0
#  define HAS_ORTHANC_PLUGIN_FIND_MATCHER  1
#else
#  define HAS_ORTHANC_PLUGIN_FIND_MATCHER  0
#endif


#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 4, 2)
#  define HAS_ORTHANC_PLUGIN_PEERS  1
#  define HAS_ORTHANC_PLUGIN_JOB    1
#else
#  define HAS_ORTHANC_PLUGIN_PEERS  0
#  define HAS_ORTHANC_PLUGIN_JOB    0
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 5, 0)
#  define HAS_ORTHANC_PLUGIN_EXCEPTION_DETAILS  1
#else
#  define HAS_ORTHANC_PLUGIN_EXCEPTION_DETAILS  0
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 5, 4)
#  define HAS_ORTHANC_PLUGIN_METRICS  1
#else
#  define HAS_ORTHANC_PLUGIN_METRICS  0
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 1, 0)
#  define HAS_ORTHANC_PLUGIN_HTTP_CLIENT  1
#else
#  define HAS_ORTHANC_PLUGIN_HTTP_CLIENT  0
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 5, 7)
#  define HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_CLIENT  1
#else
#  define HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_CLIENT  0
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 5, 7)
#  define HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_SERVER  1
#else
#  define HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_SERVER  0
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 6, 0)
#  define HAS_ORTHANC_PLUGIN_STORAGE_COMMITMENT_SCP  1
#else
#  define HAS_ORTHANC_PLUGIN_STORAGE_COMMITMENT_SCP  0
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 9, 2)
#  define HAS_ORTHANC_PLUGIN_GENERIC_CALL_REST_API  1
#else
#  define HAS_ORTHANC_PLUGIN_GENERIC_CALL_REST_API  0
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 10, 1)
#  define HAS_ORTHANC_PLUGIN_WEBDAV  1
#else
#  define HAS_ORTHANC_PLUGIN_WEBDAV  0
#endif



namespace OrthancPlugins
{
  typedef void (*RestCallback) (OrthancPluginRestOutput* output,
                                const char* url,
                                const OrthancPluginHttpRequest* request);

  void SetGlobalContext(OrthancPluginContext* context);

  void ResetGlobalContext();

  bool HasGlobalContext();

  OrthancPluginContext* GetGlobalContext();

  
  class OrthancImage;


  class MemoryBuffer : public boost::noncopyable
  {
  private:
    OrthancPluginMemoryBuffer  buffer_;

    void Check(OrthancPluginErrorCode code);

    bool CheckHttp(OrthancPluginErrorCode code);

  public:
    MemoryBuffer();

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
    // This constructor makes a copy of the given buffer in the memory
    // handled by the Orthanc core
    MemoryBuffer(const void* buffer,
                 size_t size);
#endif

    ~MemoryBuffer()
    {
      Clear();
    }

    OrthancPluginMemoryBuffer* operator*()
    {
      return &buffer_;
    }

    // This transfers ownership from "other" to "this"
    void Assign(OrthancPluginMemoryBuffer& other);

    void Swap(MemoryBuffer& other);

    OrthancPluginMemoryBuffer Release();

    const char* GetData() const
    {
      if (buffer_.size > 0)
      {
        return reinterpret_cast<const char*>(buffer_.data);
      }
      else
      {
        return NULL;
      }
    }

    size_t GetSize() const
    {
      return buffer_.size;
    }

    bool IsEmpty() const
    {
      return GetSize() == 0 || GetData() == NULL;
    }

    void Clear();

    void ToString(std::string& target) const;

    void ToJson(Json::Value& target) const;

    bool RestApiGet(const std::string& uri,
                    bool applyPlugins);

    bool RestApiGet(const std::string& uri,
                    const std::map<std::string, std::string>& httpHeaders,
                    bool applyPlugins);

    bool RestApiPost(const std::string& uri,
                     const void* body,
                     size_t bodySize,
                     bool applyPlugins);

    bool RestApiPut(const std::string& uri,
                    const void* body,
                    size_t bodySize,
                    bool applyPlugins);

    bool RestApiPost(const std::string& uri,
                     const Json::Value& body,
                     bool applyPlugins);

#if HAS_ORTHANC_PLUGIN_GENERIC_CALL_REST_API == 1
    bool RestApiPost(const std::string& uri,
                     const Json::Value& body,
                     const std::map<std::string, std::string>& httpHeaders,
                     bool applyPlugins);

    bool RestApiPost(const std::string& uri,
                     const void* body,
                     size_t bodySize,
                     const std::map<std::string, std::string>& httpHeaders,
                     bool applyPlugins);
#endif

    bool RestApiPut(const std::string& uri,
                    const Json::Value& body,
                    bool applyPlugins);

    bool RestApiPost(const std::string& uri,
                     const std::string& body,
                     bool applyPlugins)
    {
      return RestApiPost(uri, body.empty() ? NULL : body.c_str(), body.size(), applyPlugins);
    }

    bool RestApiPut(const std::string& uri,
                    const std::string& body,
                    bool applyPlugins)
    {
      return RestApiPut(uri, body.empty() ? NULL : body.c_str(), body.size(), applyPlugins);
    }

    void CreateDicom(const Json::Value& tags,
                     OrthancPluginCreateDicomFlags flags);

    void CreateDicom(const Json::Value& tags,
                     const OrthancImage& pixelData,
                     OrthancPluginCreateDicomFlags flags);

    void ReadFile(const std::string& path);

    void GetDicomQuery(const OrthancPluginWorklistQuery* query);

    void DicomToJson(Json::Value& target,
                     OrthancPluginDicomToJsonFormat format,
                     OrthancPluginDicomToJsonFlags flags,
                     uint32_t maxStringLength);

    bool HttpGet(const std::string& url,
                 const std::string& username,
                 const std::string& password);

    bool HttpPost(const std::string& url,
                  const std::string& body,
                  const std::string& username,
                  const std::string& password);

    bool HttpPut(const std::string& url,
                 const std::string& body,
                 const std::string& username,
                 const std::string& password);

    void GetDicomInstance(const std::string& instanceId);
  };


  class OrthancString : public boost::noncopyable
  {
  private:
    char*   str_;

    void Clear();

  public:
    OrthancString() :
      str_(NULL)
    {
    }

    ~OrthancString()
    {
      Clear();
    }

    // This transfers ownership, warning: The string must have been
    // allocated by the Orthanc core
    void Assign(char* str);

    const char* GetContent() const
    {
      return str_;
    }

    bool IsNullOrEmpty() const
    {
      return str_ == NULL || str_[0] == 0;
    }

    void ToString(std::string& target) const;

    void ToJson(Json::Value& target) const;
  
    void ToJsonWithoutComments(Json::Value& target) const;
};


  class OrthancConfiguration : public boost::noncopyable
  {
  private:
    Json::Value  configuration_;  // Necessarily a Json::objectValue
    std::string  path_;

    std::string GetPath(const std::string& key) const;

    void LoadConfiguration();
    
  public:
    OrthancConfiguration(); // loads the full Orthanc configuration

    explicit OrthancConfiguration(bool load);

    explicit OrthancConfiguration(const Json::Value& configuration, const std::string& path);  // e.g. to load a section from a default json content

    const Json::Value& GetJson() const
    {
      return configuration_;
    }

    bool IsSection(const std::string& key) const;

    void GetSection(OrthancConfiguration& target,
                    const std::string& key) const;

    bool LookupStringValue(std::string& target,
                           const std::string& key) const;
    
    bool LookupIntegerValue(int& target,
                            const std::string& key) const;

    bool LookupUnsignedIntegerValue(unsigned int& target,
                                    const std::string& key) const;

    bool LookupBooleanValue(bool& target,
                            const std::string& key) const;

    bool LookupFloatValue(float& target,
                          const std::string& key) const;

    bool LookupListOfStrings(std::list<std::string>& target,
                             const std::string& key,
                             bool allowSingleString) const;

    bool LookupSetOfStrings(std::set<std::string>& target,
                            const std::string& key,
                            bool allowSingleString) const;

    std::string GetStringValue(const std::string& key,
                               const std::string& defaultValue) const;

    int GetIntegerValue(const std::string& key,
                        int defaultValue) const;

    unsigned int GetUnsignedIntegerValue(const std::string& key,
                                         unsigned int defaultValue) const;

    bool GetBooleanValue(const std::string& key,
                         bool defaultValue) const;

    float GetFloatValue(const std::string& key,
                        float defaultValue) const;

    void GetDictionary(std::map<std::string, std::string>& target,
                       const std::string& key) const;
  };

  class OrthancImage : public boost::noncopyable
  {
  private:
    OrthancPluginImage*    image_;

    void Clear();

    void CheckImageAvailable() const;

  public:
    OrthancImage();

    explicit OrthancImage(OrthancPluginImage* image);

    OrthancImage(OrthancPluginPixelFormat  format,
                 uint32_t                  width,
                 uint32_t                  height);

    OrthancImage(OrthancPluginPixelFormat  format,
                 uint32_t                  width,
                 uint32_t                  height,
                 uint32_t                  pitch,
                 void*                     buffer);

    ~OrthancImage()
    {
      Clear();
    }

    void UncompressPngImage(const void* data,
                            size_t size);

    void UncompressJpegImage(const void* data,
                             size_t size);

    void DecodeDicomImage(const void* data,
                          size_t size,
                          unsigned int frame);

    OrthancPluginPixelFormat GetPixelFormat() const;

    unsigned int GetWidth() const;

    unsigned int GetHeight() const;

    unsigned int GetPitch() const;
    
    void* GetBuffer() const;

    const OrthancPluginImage* GetObject() const
    {
      return image_;
    }

    void CompressPngImage(MemoryBuffer& target) const;

    void CompressJpegImage(MemoryBuffer& target,
                           uint8_t quality) const;

    void AnswerPngImage(OrthancPluginRestOutput* output) const;

    void AnswerJpegImage(OrthancPluginRestOutput* output,
                         uint8_t quality) const;
    
    void* GetWriteableBuffer();

    OrthancPluginImage* Release();
  };


#if HAS_ORTHANC_PLUGIN_FIND_MATCHER == 1
  class FindMatcher : public boost::noncopyable
  {
  private:
    OrthancPluginFindMatcher*          matcher_;
    const OrthancPluginWorklistQuery*  worklist_;

    void SetupDicom(const void*            query,
                    uint32_t               size);

  public:
    explicit FindMatcher(const OrthancPluginWorklistQuery*  worklist);

    FindMatcher(const void*  query,
                uint32_t     size)
    {
      SetupDicom(query, size);
    }

    explicit FindMatcher(const MemoryBuffer&  dicom)
    {
      SetupDicom(dicom.GetData(), dicom.GetSize());
    }

    ~FindMatcher();

    bool IsMatch(const void*  dicom,
                 uint32_t     size) const;

    bool IsMatch(const MemoryBuffer& dicom) const
    {
      return IsMatch(dicom.GetData(), dicom.GetSize());
    }
  };
#endif


  bool ReadJson(Json::Value& target,
                const std::string& source);
  
  bool ReadJson(Json::Value& target,
                const void* buffer,
                size_t size);

  bool ReadJsonWithoutComments(Json::Value& target,
                               const std::string& source);  

  bool ReadJsonWithoutComments(Json::Value& target,
                               const void* buffer,
                               size_t size);

  void WriteFastJson(std::string& target,
                     const Json::Value& source);

  void WriteStyledJson(std::string& target,
                       const Json::Value& source);

  bool RestApiGet(Json::Value& result,
                  const std::string& uri,
                  bool applyPlugins);

  bool RestApiGet(Json::Value& result,
                  const std::string& uri,
                  const std::map<std::string, std::string>& httpHeaders,
                  bool applyPlugins);

  bool RestApiGetString(std::string& result,
                        const std::string& uri,
                        bool applyPlugins);

  bool RestApiGetString(std::string& result,
                        const std::string& uri,
                        const std::map<std::string, std::string>& httpHeaders,
                        bool applyPlugins);

  bool RestApiPost(std::string& result,
                   const std::string& uri,
                   const void* body,
                   size_t bodySize,
                   bool applyPlugins);

  bool RestApiPost(Json::Value& result,
                   const std::string& uri,
                   const void* body,
                   size_t bodySize,
                   bool applyPlugins);

#if HAS_ORTHANC_PLUGIN_GENERIC_CALL_REST_API == 1
  bool RestApiPost(Json::Value& result,
                   const std::string& uri,
                   const Json::Value& body,
                   const std::map<std::string, std::string>& httpHeaders,
                   bool applyPlugins);
#endif

  bool RestApiPost(Json::Value& result,
                   const std::string& uri,
                   const Json::Value& body,
                   bool applyPlugins);

  inline bool RestApiPost(Json::Value& result,
                          const std::string& uri,
                          const std::string& body,
                          bool applyPlugins)
  {
    return RestApiPost(result, uri, body.empty() ? NULL : body.c_str(),
                       body.size(), applyPlugins);
  }

  inline bool RestApiPost(Json::Value& result,
                          const std::string& uri,
                          const MemoryBuffer& body,
                          bool applyPlugins)
  {
    return RestApiPost(result, uri, body.GetData(),
                       body.GetSize(), applyPlugins);
  }

  bool RestApiPut(Json::Value& result,
                  const std::string& uri,
                  const void* body,
                  size_t bodySize,
                  bool applyPlugins);

  bool RestApiPut(Json::Value& result,
                  const std::string& uri,
                  const Json::Value& body,
                  bool applyPlugins);

  inline bool RestApiPut(Json::Value& result,
                         const std::string& uri,
                         const std::string& body,
                         bool applyPlugins)
  {
    return RestApiPut(result, uri, body.empty() ? NULL : body.c_str(),
                      body.size(), applyPlugins);
  }

  bool RestApiDelete(const std::string& uri,
                     bool applyPlugins);

  bool HttpDelete(const std::string& url,
                  const std::string& username,
                  const std::string& password);

  void AnswerJson(const Json::Value& value,
                  OrthancPluginRestOutput* output);

  void AnswerString(const std::string& answer,
                    const char* mimeType,
                    OrthancPluginRestOutput* output);

  void AnswerHttpError(uint16_t httpError,
                       OrthancPluginRestOutput* output);

  void AnswerMethodNotAllowed(OrthancPluginRestOutput* output, const char* allowedMethods);

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 5, 0)
  const char* AutodetectMimeType(const std::string& path);
#endif

  void LogError(const std::string& message);

  void LogWarning(const std::string& message);

  void LogInfo(const std::string& message);

  void ReportMinimalOrthancVersion(unsigned int major,
                                   unsigned int minor,
                                   unsigned int revision);
  
  bool CheckMinimalOrthancVersion(unsigned int major,
                                  unsigned int minor,
                                  unsigned int revision);

  bool CheckMinimalVersion(const char* version,
                           unsigned int major,
                           unsigned int minor,
                           unsigned int revision);

  namespace Internals
  {
    template <RestCallback Callback>
    static OrthancPluginErrorCode Protect(OrthancPluginRestOutput* output,
                                          const char* url,
                                          const OrthancPluginHttpRequest* request)
    {
      try
      {
        Callback(output, url, request);
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
  }

  
  template <RestCallback Callback>
  void RegisterRestCallback(const std::string& uri,
                            bool isThreadSafe)
  {
    if (isThreadSafe)
    {
      OrthancPluginRegisterRestCallbackNoLock
        (GetGlobalContext(), uri.c_str(), Internals::Protect<Callback>);
    }
    else
    {
      OrthancPluginRegisterRestCallback
        (GetGlobalContext(), uri.c_str(), Internals::Protect<Callback>);
    }
  }


#if HAS_ORTHANC_PLUGIN_PEERS == 1
  class OrthancPeers : public boost::noncopyable
  {
  private:
    typedef std::map<std::string, uint32_t>   Index;

    OrthancPluginPeers   *peers_;
    Index                 index_;
    uint32_t              timeout_;

    size_t GetPeerIndex(const std::string& name) const;

  public:
    OrthancPeers();

    ~OrthancPeers();

    uint32_t GetTimeout() const
    {
      return timeout_;
    }

    void SetTimeout(uint32_t timeout)
    {
      timeout_ = timeout;
    }

    bool LookupName(size_t& target,
                    const std::string& name) const;

    std::string GetPeerName(size_t index) const;

    std::string GetPeerUrl(size_t index) const;

    std::string GetPeerUrl(const std::string& name) const;

    size_t GetPeersCount() const
    {
      return index_.size();
    }

    bool LookupUserProperty(std::string& value,
                            size_t index,
                            const std::string& key) const;

    bool LookupUserProperty(std::string& value,
                            const std::string& peer,
                            const std::string& key) const;

    bool DoGet(MemoryBuffer& target,
               size_t index,
               const std::string& uri,
               const std::map<std::string, std::string>& headers) const;

    bool DoGet(MemoryBuffer& target,
               const std::string& name,
               const std::string& uri,
               const std::map<std::string, std::string>& headers) const;

    bool DoGet(Json::Value& target,
               size_t index,
               const std::string& uri,
               const std::map<std::string, std::string>& headers) const;

    bool DoGet(Json::Value& target,
               const std::string& name,
               const std::string& uri,
               const std::map<std::string, std::string>& headers) const;

    bool DoPost(MemoryBuffer& target,
                size_t index,
                const std::string& uri,
                const std::string& body,
                const std::map<std::string, std::string>& headers) const;

    bool DoPost(MemoryBuffer& target,
                const std::string& name,
                const std::string& uri,
                const std::string& body,
                const std::map<std::string, std::string>& headers) const;

    bool DoPost(Json::Value& target,
                size_t index,
                const std::string& uri,
                const std::string& body,
                const std::map<std::string, std::string>& headers) const;

    bool DoPost(Json::Value& target,
                const std::string& name,
                const std::string& uri,
                const std::string& body,
                const std::map<std::string, std::string>& headers) const;

    bool DoPut(size_t index,
               const std::string& uri,
               const std::string& body,
               const std::map<std::string, std::string>& headers) const;

    bool DoPut(const std::string& name,
               const std::string& uri,
               const std::string& body,
               const std::map<std::string, std::string>& headers) const;

    bool DoDelete(size_t index,
                  const std::string& uri,
                  const std::map<std::string, std::string>& headers) const;

    bool DoDelete(const std::string& name,
                  const std::string& uri,
                  const std::map<std::string, std::string>& headers) const;
  };
#endif



#if HAS_ORTHANC_PLUGIN_JOB == 1
  class OrthancJob : public boost::noncopyable
  {
  private:
    std::string   jobType_;
    std::string   content_;
    bool          hasSerialized_;
    std::string   serialized_;
    float         progress_;

    static void CallbackFinalize(void* job);

    static float CallbackGetProgress(void* job);

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 11, 3)
    static OrthancPluginErrorCode CallbackGetContent(OrthancPluginMemoryBuffer* target,
                                                     void* job);
#else
    static const char* CallbackGetContent(void* job);
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 11, 3)
    static int32_t CallbackGetSerialized(OrthancPluginMemoryBuffer* target,
                                         void* job);
#else
    static const char* CallbackGetSerialized(void* job);
#endif

    static OrthancPluginJobStepStatus CallbackStep(void* job);

    static OrthancPluginErrorCode CallbackStop(void* job,
                                               OrthancPluginJobStopReason reason);

    static OrthancPluginErrorCode CallbackReset(void* job);

  protected:
    void ClearContent();

    void UpdateContent(const Json::Value& content);

    void ClearSerialized();

    void UpdateSerialized(const Json::Value& serialized);

    void UpdateProgress(float progress);
    
  public:
    explicit OrthancJob(const std::string& jobType);
    
    virtual ~OrthancJob()
    {
    }

    virtual OrthancPluginJobStepStatus Step() = 0;

    virtual void Stop(OrthancPluginJobStopReason reason) = 0;
    
    virtual void Reset() = 0;

    static OrthancPluginJob* Create(OrthancJob* job /* takes ownership */);

    static std::string Submit(OrthancJob* job /* takes ownership */,
                              int priority);

    static void SubmitAndWait(Json::Value& result,
                              OrthancJob* job /* takes ownership */,
                              int priority);

    // Submit a job from a POST on the REST API with the same
    // conventions as in the Orthanc core (according to the
    // "Synchronous" and "Priority" options)
    static void SubmitFromRestApiPost(OrthancPluginRestOutput* output,
                                      const Json::Value& body,
                                      OrthancJob* job);
  };
#endif


#if HAS_ORTHANC_PLUGIN_METRICS == 1
  inline void SetMetricsValue(char* name,
                              float value)
  {
    OrthancPluginSetMetricsValue(GetGlobalContext(), name,
                                 value, OrthancPluginMetricsType_Default);
  }

  class MetricsTimer : public boost::noncopyable
  {
  private:
    std::string               name_;
    boost::posix_time::ptime  start_;

  public:
    explicit MetricsTimer(const char* name);

    ~MetricsTimer();
  };
#endif


#if HAS_ORTHANC_PLUGIN_HTTP_CLIENT == 1
  class HttpClient : public boost::noncopyable
  {
  public:
    typedef std::map<std::string, std::string>  HttpHeaders;

    class IRequestBody : public boost::noncopyable
    {
    public:
      virtual ~IRequestBody()
      {
      }

      virtual bool ReadNextChunk(std::string& chunk) = 0;
    };


    class IAnswer : public boost::noncopyable
    {
    public:
      virtual ~IAnswer()
      {
      }

      virtual void AddHeader(const std::string& key,
                             const std::string& value) = 0;

      virtual void AddChunk(const void* data,
                            size_t size) = 0;
    };


  private:
    class RequestBodyWrapper;

    uint16_t                 httpStatus_;
    OrthancPluginHttpMethod  method_;
    std::string              url_;
    HttpHeaders              headers_;
    std::string              username_;
    std::string              password_;
    uint32_t                 timeout_;
    std::string              certificateFile_;
    std::string              certificateKeyFile_;
    std::string              certificateKeyPassword_;
    bool                     pkcs11_;
    std::string              fullBody_;
    IRequestBody*            chunkedBody_;
    bool                     allowChunkedTransfers_;

#if HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_CLIENT == 1
    void ExecuteWithStream(uint16_t& httpStatus,  // out
                           IAnswer& answer,       // out
                           IRequestBody& body) const;
#endif

    void ExecuteWithoutStream(uint16_t& httpStatus,        // out
                              HttpHeaders& answerHeaders,  // out
                              std::string& answerBody,     // out
                              const std::string& body) const;
    
  public:
    HttpClient();

    uint16_t GetHttpStatus() const
    {
      return httpStatus_;
    }

    void SetMethod(OrthancPluginHttpMethod method)
    {
      method_ = method;
    }

    const std::string& GetUrl() const
    {
      return url_;
    }

    void SetUrl(const std::string& url)
    {
      url_ = url;
    }

    void SetHeaders(const HttpHeaders& headers)
    {
      headers_ = headers;
    }

    void AddHeader(const std::string& key,
                   const std::string& value)
    {
      headers_[key] = value;
    }

    void AddHeaders(const HttpHeaders& headers);

    void SetCredentials(const std::string& username,
                        const std::string& password);

    void ClearCredentials();

    void SetTimeout(unsigned int timeout)  // 0 for default timeout
    {
      timeout_ = timeout;
    }

    void SetCertificate(const std::string& certificateFile,
                        const std::string& keyFile,
                        const std::string& keyPassword);

    void ClearCertificate();

    void SetPkcs11(bool pkcs11)
    {
      pkcs11_ = pkcs11;
    }

    void ClearBody();

    void SwapBody(std::string& body);

    void SetBody(const std::string& body);

    void SetBody(IRequestBody& body);

    // This function can be used to disable chunked transfers if the
    // remote server is Orthanc with a version <= 1.5.6.
    void SetChunkedTransfersAllowed(bool allow)
    {
      allowChunkedTransfers_ = allow;
    }

    bool IsChunkedTransfersAllowed() const
    {
      return allowChunkedTransfers_;
    }

    void Execute(IAnswer& answer);

    void Execute(HttpHeaders& answerHeaders /* out */,
                 std::string& answerBody /* out */);

    void Execute(HttpHeaders& answerHeaders /* out */,
                 Json::Value& answerBody /* out */);

    void Execute();
  };
#endif



  class IChunkedRequestReader : public boost::noncopyable
  {
  public:
    virtual ~IChunkedRequestReader()
    {
    }

    virtual void AddChunk(const void* data,
                          size_t size) = 0;

    virtual void Execute(OrthancPluginRestOutput* output) = 0;
  };


  typedef IChunkedRequestReader* (*ChunkedRestCallback) (const char* url,
                                                         const OrthancPluginHttpRequest* request);


  namespace Internals
  {
    void NullRestCallback(OrthancPluginRestOutput* output,
                          const char* url,
                          const OrthancPluginHttpRequest* request);
  
    IChunkedRequestReader *NullChunkedRestCallback(const char* url,
                                                   const OrthancPluginHttpRequest* request);


#if HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_SERVER == 1
    template <ChunkedRestCallback Callback>
    static OrthancPluginErrorCode ChunkedProtect(OrthancPluginServerChunkedRequestReader** reader,
                                                const char* url,
                                                const OrthancPluginHttpRequest* request)
    {
      try
      {
        if (reader == NULL)
        {
          return OrthancPluginErrorCode_InternalError;
        }
        else
        {
          *reader = reinterpret_cast<OrthancPluginServerChunkedRequestReader*>(Callback(url, request));
          if (*reader == NULL)
          {
            return OrthancPluginErrorCode_Plugin;
          }
          else
          {
            return OrthancPluginErrorCode_Success;
          }
        }
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

    OrthancPluginErrorCode ChunkedRequestReaderAddChunk(
      OrthancPluginServerChunkedRequestReader* reader,
      const void*                              data,
      uint32_t                                 size);

    OrthancPluginErrorCode ChunkedRequestReaderExecute(
      OrthancPluginServerChunkedRequestReader* reader,
      OrthancPluginRestOutput*                 output);

    void ChunkedRequestReaderFinalize(
      OrthancPluginServerChunkedRequestReader* reader);

#else  

    OrthancPluginErrorCode ChunkedRestCompatibility(OrthancPluginRestOutput* output,
                                                    const char* url,
                                                    const OrthancPluginHttpRequest* request,
                                                    RestCallback GetHandler,
                                                    ChunkedRestCallback PostHandler,
                                                    RestCallback DeleteHandler,
                                                    ChunkedRestCallback PutHandler);

    template<
      RestCallback         GetHandler,
      ChunkedRestCallback  PostHandler,
      RestCallback         DeleteHandler,
      ChunkedRestCallback  PutHandler
      >
    inline OrthancPluginErrorCode ChunkedRestCompatibility(OrthancPluginRestOutput* output,
                                                           const char* url,
                                                           const OrthancPluginHttpRequest* request)
    {
      return ChunkedRestCompatibility(output, url, request, GetHandler,
                                      PostHandler, DeleteHandler, PutHandler);
    }
#endif
  }



  // NB: We use a templated class instead of a templated function, because
  // default values are only available in functions since C++11
  template<
    RestCallback         GetHandler    = Internals::NullRestCallback,
    ChunkedRestCallback  PostHandler   = Internals::NullChunkedRestCallback,
    RestCallback         DeleteHandler = Internals::NullRestCallback,
    ChunkedRestCallback  PutHandler    = Internals::NullChunkedRestCallback
    >
  class ChunkedRestRegistration : public boost::noncopyable
  {
  public:
    static void Apply(const std::string& uri)
    {
#if HAS_ORTHANC_PLUGIN_CHUNKED_HTTP_SERVER == 1
      OrthancPluginRegisterChunkedRestCallback(
        GetGlobalContext(), uri.c_str(),
        GetHandler == Internals::NullRestCallback         ? NULL : Internals::Protect<GetHandler>,
        PostHandler == Internals::NullChunkedRestCallback ? NULL : Internals::ChunkedProtect<PostHandler>,
        DeleteHandler == Internals::NullRestCallback      ? NULL : Internals::Protect<DeleteHandler>,
        PutHandler == Internals::NullChunkedRestCallback  ? NULL : Internals::ChunkedProtect<PutHandler>,
        Internals::ChunkedRequestReaderAddChunk,
        Internals::ChunkedRequestReaderExecute,
        Internals::ChunkedRequestReaderFinalize);
#else
      OrthancPluginRegisterRestCallbackNoLock(
        GetGlobalContext(), uri.c_str(), 
        Internals::ChunkedRestCompatibility<GetHandler, PostHandler, DeleteHandler, PutHandler>);
#endif
    }
  };

  

#if HAS_ORTHANC_PLUGIN_STORAGE_COMMITMENT_SCP == 1
  class IStorageCommitmentScpHandler : public boost::noncopyable
  {
  public:
    virtual ~IStorageCommitmentScpHandler()
    {
    }
    
    virtual OrthancPluginStorageCommitmentFailureReason Lookup(const std::string& sopClassUid,
                                                               const std::string& sopInstanceUid) = 0;
    
    static OrthancPluginErrorCode Lookup(OrthancPluginStorageCommitmentFailureReason* target,
                                         void* rawHandler,
                                         const char* sopClassUid,
                                         const char* sopInstanceUid);

    static void Destructor(void* rawHandler);
  };
#endif


  class DicomInstance : public boost::noncopyable
  {
  private:
    bool toFree_;

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 6, 1)    
    const OrthancPluginDicomInstance*  instance_;
#else
    OrthancPluginDicomInstance*  instance_;
#endif
    
  public:
#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 6, 1)    
    explicit DicomInstance(const OrthancPluginDicomInstance* instance);
#else
    explicit DicomInstance(OrthancPluginDicomInstance* instance);
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
    DicomInstance(const void* buffer,
                  size_t size);
#endif

    ~DicomInstance();

    const OrthancPluginDicomInstance* GetObject() const
    {
      return instance_;
    }

    std::string GetRemoteAet() const;

    const void* GetBuffer() const
    {
      return OrthancPluginGetInstanceData(GetGlobalContext(), instance_);
    }

    size_t GetSize() const
    {
      return static_cast<size_t>(OrthancPluginGetInstanceSize(GetGlobalContext(), instance_));
    }

    void GetJson(Json::Value& target) const;

    void GetSimplifiedJson(Json::Value& target) const;

    OrthancPluginInstanceOrigin GetOrigin() const
    {
      return OrthancPluginGetInstanceOrigin(GetGlobalContext(), instance_);
    }

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 6, 1)
    std::string GetTransferSyntaxUid() const;
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 6, 1)
    bool HasPixelData() const;
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
    unsigned int GetFramesCount() const
    {
      return OrthancPluginGetInstanceFramesCount(GetGlobalContext(), instance_);
    }
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
    void GetRawFrame(std::string& target,
                     unsigned int frameIndex) const;
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
    OrthancImage* GetDecodedFrame(unsigned int frameIndex) const;
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
    void Serialize(std::string& target) const;
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 7, 0)
    static DicomInstance* Transcode(const void* buffer,
                                    size_t size,
                                    const std::string& transferSyntax);
#endif

#if ORTHANC_PLUGINS_VERSION_IS_ABOVE(1, 12, 1)
    static DicomInstance* Load(const std::string& instanceId,
                               OrthancPluginLoadDicomInstanceMode mode);
#endif
  };

// helper method to convert Http headers from the plugin SDK to a std::map
void GetHttpHeaders(std::map<std::string, std::string>& result, const OrthancPluginHttpRequest* request);

#if HAS_ORTHANC_PLUGIN_WEBDAV == 1
  class IWebDavCollection : public boost::noncopyable
  {
  public:
    class FileInfo
    {
    private:
      std::string  name_;
      uint64_t     contentSize_;
      std::string  mime_;
      std::string  dateTime_;

    public:
      FileInfo(const std::string& name,
               uint64_t contentSize,
               const std::string& dateTime) :
        name_(name),
        contentSize_(contentSize),
        dateTime_(dateTime)
      {
      }

      const std::string& GetName() const
      {
        return name_;
      }

      uint64_t GetContentSize() const
      {
        return contentSize_;
      }

      void SetMimeType(const std::string& mime)
      {
        mime_ = mime;
      }

      const std::string& GetMimeType() const
      {
        return mime_;
      }

      const std::string& GetDateTime() const
      {
        return dateTime_;
      }
    };
  
    class FolderInfo
    {
    private:
      std::string  name_;
      std::string  dateTime_;

    public:
      FolderInfo(const std::string& name,
                 const std::string& dateTime) :
        name_(name),
        dateTime_(dateTime)
      {
      }

      const std::string& GetName() const
      {
        return name_;
      }

      const std::string& GetDateTime() const
      {
        return dateTime_;
      }
    };
  
    virtual ~IWebDavCollection()
    {
    }

    virtual bool IsExistingFolder(const std::vector<std::string>& path) = 0;

    virtual bool ListFolder(std::list<FileInfo>& files,
                            std::list<FolderInfo>& subfolders,
                            const std::vector<std::string>& path) = 0;
  
    virtual bool GetFile(std::string& content /* out */,
                         std::string& mime /* out */,
                         std::string& dateTime /* out */,
                         const std::vector<std::string>& path) = 0;

    virtual bool StoreFile(const std::vector<std::string>& path,
                           const void* data,
                           size_t size) = 0;

    virtual bool CreateFolder(const std::vector<std::string>& path) = 0;

    virtual bool DeleteItem(const std::vector<std::string>& path) = 0;

    static void Register(const std::string& uri,
                         IWebDavCollection& collection);
  };
#endif

  void SetRootUri(const std::string& pluginIdentifier,
                  const std::string& uri);

  void SetDescription(const std::string& pluginIdentifier,
                      const std::string& description);

  void ExtendOrthancExplorer(const std::string& pluginIdentifier,
                             const std::string& javascript);
}
