/**
 * \mainpage
 *
 * This C/C++ SDK allows external developers to create plugins that
 * can be loaded into Orthanc to extend its functionality. Each
 * Orthanc plugin must expose 4 public functions with the following
 * signatures:
 * 
 * -# <tt>int32_t OrthancPluginInitialize(const OrthancPluginContext* context)</tt>:
 *    This function is invoked by Orthanc when it loads the plugin on startup.
 *    The plugin must:
 *    - Check its compatibility with the Orthanc version using
 *      ::OrthancPluginCheckVersion().
 *    - Store the context pointer so that it can use the plugin 
 *      services of Orthanc.
 *    - Register all its REST callbacks using ::OrthancPluginRegisterRestCallback().
 *    - Possibly register its callback for received DICOM instances using ::OrthancPluginRegisterOnStoredInstanceCallback().
 *    - Possibly register its callback for changes to the DICOM store using ::OrthancPluginRegisterOnChangeCallback().
 *    - Possibly register a custom storage area using ::OrthancPluginRegisterStorageArea2().
 *    - Possibly register a custom database back-end area using OrthancPluginRegisterDatabaseBackendV3().
 *    - Possibly register a handler for C-Find SCP using OrthancPluginRegisterFindCallback().
 *    - Possibly register a handler for C-Find SCP against DICOM worklists using OrthancPluginRegisterWorklistCallback().
 *    - Possibly register a handler for C-Move SCP using OrthancPluginRegisterMoveCallback().
 *    - Possibly register a custom decoder for DICOM images using OrthancPluginRegisterDecodeImageCallback().
 *    - Possibly register a callback to filter incoming HTTP requests using OrthancPluginRegisterIncomingHttpRequestFilter2().
 *    - Possibly register a callback to unserialize jobs using OrthancPluginRegisterJobsUnserializer().
 *    - Possibly register a callback to refresh its metrics using OrthancPluginRegisterRefreshMetricsCallback().
 *    - Possibly register a callback to answer chunked HTTP transfers using ::OrthancPluginRegisterChunkedRestCallback().
 *    - Possibly register a callback for Storage Commitment SCP using ::OrthancPluginRegisterStorageCommitmentScpCallback().
 *    - Possibly register a callback to keep/discard/modify incoming DICOM instances using OrthancPluginRegisterReceivedInstanceCallback().
 *    - Possibly register a custom transcoder for DICOM images using OrthancPluginRegisterTranscoderCallback().
 *    - Possibly register a callback to discard instances received through DICOM C-STORE using OrthancPluginRegisterIncomingCStoreInstanceFilter().
 *    - Possibly register a callback to branch a WebDAV virtual filesystem using OrthancPluginRegisterWebDavCollection().
 * -# <tt>void OrthancPluginFinalize()</tt>:
 *    This function is invoked by Orthanc during its shutdown. The plugin
 *    must free all its memory.
 * -# <tt>const char* OrthancPluginGetName()</tt>:
 *    The plugin must return a short string to identify itself.
 * -# <tt>const char* OrthancPluginGetVersion()</tt>:
 *    The plugin must return a string containing its version number.
 *
 * The name and the version of a plugin is only used to prevent it
 * from being loaded twice. Note that, in C++, it is mandatory to
 * declare these functions within an <tt>extern "C"</tt> section.
 * 
 * To ensure multi-threading safety, the various REST callbacks are
 * guaranteed to be executed in mutual exclusion since Orthanc
 * 0.8.5. If this feature is undesired (notably when developing
 * high-performance plugins handling simultaneous requests), use
 * ::OrthancPluginRegisterRestCallbackNoLock().
 **/



/**
 * @defgroup Images Images and compression
 * @brief Functions to deal with images and compressed buffers.
 *
 * @defgroup REST REST
 * @brief Functions to answer REST requests in a callback.
 *
 * @defgroup Callbacks Callbacks
 * @brief Functions to register and manage callbacks by the plugins.
 *
 * @defgroup DicomCallbacks DicomCallbacks
 * @brief Functions to register and manage DICOM callbacks (worklists, C-FIND, C-MOVE, storage commitment).
 *
 * @defgroup Orthanc Orthanc
 * @brief Functions to access the content of the Orthanc server.
 *
 * @defgroup DicomInstance DicomInstance
 * @brief Functions to access DICOM images that are managed by the Orthanc core.
 **/



/**
 * @defgroup Toolbox Toolbox
 * @brief Generic functions to help with the creation of plugins.
 **/



/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2022 Osimis S.A., Belgium
 * Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#include <stdio.h>
#include <string.h>

#ifdef WIN32
#  define ORTHANC_PLUGINS_API __declspec(dllexport)
#elif __GNUC__ >= 4
#  define ORTHANC_PLUGINS_API __attribute__ ((visibility ("default")))
#else
#  define ORTHANC_PLUGINS_API
#endif

#define ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER     1
#define ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER     11
#define ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER  2


#if !defined(ORTHANC_PLUGINS_VERSION_IS_ABOVE)
#define ORTHANC_PLUGINS_VERSION_IS_ABOVE(major, minor, revision)        \
  (ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER > major ||                      \
   (ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER == major &&                    \
    (ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER > minor ||                    \
     (ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER == minor &&                  \
      ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER >= revision))))
#endif



/********************************************************************
 ** Check that function inlining is properly supported. The use of
 ** inlining is required, to avoid the duplication of object code
 ** between two compilation modules that would use the Orthanc Plugin
 ** API.
 ********************************************************************/

/* If the auto-detection of the "inline" keyword below does not work
   automatically and that your compiler is known to properly support
   inlining, uncomment the following #define and adapt the definition
   of "static inline". */

/* #define ORTHANC_PLUGIN_INLINE static inline */

#ifndef ORTHANC_PLUGIN_INLINE
#  if __STDC_VERSION__ >= 199901L
/*   This is C99 or above: http://predef.sourceforge.net/prestd.html */
#    define ORTHANC_PLUGIN_INLINE static inline
#  elif defined(__cplusplus)
/*   This is C++ */
#    define ORTHANC_PLUGIN_INLINE static inline
#  elif defined(__GNUC__)
/*   This is GCC running in C89 mode */
#    define ORTHANC_PLUGIN_INLINE static __inline
#  elif defined(_MSC_VER)
/*   This is Visual Studio running in C89 mode */
#    define ORTHANC_PLUGIN_INLINE static __inline
#  else
#    error Your compiler is not known to support the "inline" keyword
#  endif
#endif



/********************************************************************
 ** Inclusion of standard libraries.
 ********************************************************************/

/**
 * For Microsoft Visual Studio, a compatibility "stdint.h" can be
 * downloaded at the following URL:
 * https://hg.orthanc-server.com/orthanc/raw-file/tip/Resources/ThirdParty/VisualStudio/stdint.h
 **/
#include <stdint.h>

#include <stdlib.h>



/********************************************************************
 ** Definition of the Orthanc Plugin API.
 ********************************************************************/

/** @{ */

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * The various error codes that can be returned by the Orthanc core.
   **/
  typedef enum
  {
    OrthancPluginErrorCode_InternalError = -1    /*!< Internal error */,
    OrthancPluginErrorCode_Success = 0    /*!< Success */,
    OrthancPluginErrorCode_Plugin = 1    /*!< Error encountered within the plugin engine */,
    OrthancPluginErrorCode_NotImplemented = 2    /*!< Not implemented yet */,
    OrthancPluginErrorCode_ParameterOutOfRange = 3    /*!< Parameter out of range */,
    OrthancPluginErrorCode_NotEnoughMemory = 4    /*!< The server hosting Orthanc is running out of memory */,
    OrthancPluginErrorCode_BadParameterType = 5    /*!< Bad type for a parameter */,
    OrthancPluginErrorCode_BadSequenceOfCalls = 6    /*!< Bad sequence of calls */,
    OrthancPluginErrorCode_InexistentItem = 7    /*!< Accessing an inexistent item */,
    OrthancPluginErrorCode_BadRequest = 8    /*!< Bad request */,
    OrthancPluginErrorCode_NetworkProtocol = 9    /*!< Error in the network protocol */,
    OrthancPluginErrorCode_SystemCommand = 10    /*!< Error while calling a system command */,
    OrthancPluginErrorCode_Database = 11    /*!< Error with the database engine */,
    OrthancPluginErrorCode_UriSyntax = 12    /*!< Badly formatted URI */,
    OrthancPluginErrorCode_InexistentFile = 13    /*!< Inexistent file */,
    OrthancPluginErrorCode_CannotWriteFile = 14    /*!< Cannot write to file */,
    OrthancPluginErrorCode_BadFileFormat = 15    /*!< Bad file format */,
    OrthancPluginErrorCode_Timeout = 16    /*!< Timeout */,
    OrthancPluginErrorCode_UnknownResource = 17    /*!< Unknown resource */,
    OrthancPluginErrorCode_IncompatibleDatabaseVersion = 18    /*!< Incompatible version of the database */,
    OrthancPluginErrorCode_FullStorage = 19    /*!< The file storage is full */,
    OrthancPluginErrorCode_CorruptedFile = 20    /*!< Corrupted file (e.g. inconsistent MD5 hash) */,
    OrthancPluginErrorCode_InexistentTag = 21    /*!< Inexistent tag */,
    OrthancPluginErrorCode_ReadOnly = 22    /*!< Cannot modify a read-only data structure */,
    OrthancPluginErrorCode_IncompatibleImageFormat = 23    /*!< Incompatible format of the images */,
    OrthancPluginErrorCode_IncompatibleImageSize = 24    /*!< Incompatible size of the images */,
    OrthancPluginErrorCode_SharedLibrary = 25    /*!< Error while using a shared library (plugin) */,
    OrthancPluginErrorCode_UnknownPluginService = 26    /*!< Plugin invoking an unknown service */,
    OrthancPluginErrorCode_UnknownDicomTag = 27    /*!< Unknown DICOM tag */,
    OrthancPluginErrorCode_BadJson = 28    /*!< Cannot parse a JSON document */,
    OrthancPluginErrorCode_Unauthorized = 29    /*!< Bad credentials were provided to an HTTP request */,
    OrthancPluginErrorCode_BadFont = 30    /*!< Badly formatted font file */,
    OrthancPluginErrorCode_DatabasePlugin = 31    /*!< The plugin implementing a custom database back-end does not fulfill the proper interface */,
    OrthancPluginErrorCode_StorageAreaPlugin = 32    /*!< Error in the plugin implementing a custom storage area */,
    OrthancPluginErrorCode_EmptyRequest = 33    /*!< The request is empty */,
    OrthancPluginErrorCode_NotAcceptable = 34    /*!< Cannot send a response which is acceptable according to the Accept HTTP header */,
    OrthancPluginErrorCode_NullPointer = 35    /*!< Cannot handle a NULL pointer */,
    OrthancPluginErrorCode_DatabaseUnavailable = 36    /*!< The database is currently not available (probably a transient situation) */,
    OrthancPluginErrorCode_CanceledJob = 37    /*!< This job was canceled */,
    OrthancPluginErrorCode_BadGeometry = 38    /*!< Geometry error encountered in Stone */,
    OrthancPluginErrorCode_SslInitialization = 39    /*!< Cannot initialize SSL encryption, check out your certificates */,
    OrthancPluginErrorCode_DiscontinuedAbi = 40    /*!< Calling a function that has been removed from the Orthanc Framework */,
    OrthancPluginErrorCode_BadRange = 41    /*!< Incorrect range request */,
    OrthancPluginErrorCode_DatabaseCannotSerialize = 42    /*!< Database could not serialize access due to concurrent update, the transaction should be retried */,
    OrthancPluginErrorCode_Revision = 43    /*!< A bad revision number was provided, which might indicate conflict between multiple writers */,
    OrthancPluginErrorCode_MainDicomTagsMultiplyDefined = 44    /*!< A main DICOM Tag has been defined multiple times for the same resource level */,
    OrthancPluginErrorCode_SQLiteNotOpened = 1000    /*!< SQLite: The database is not opened */,
    OrthancPluginErrorCode_SQLiteAlreadyOpened = 1001    /*!< SQLite: Connection is already open */,
    OrthancPluginErrorCode_SQLiteCannotOpen = 1002    /*!< SQLite: Unable to open the database */,
    OrthancPluginErrorCode_SQLiteStatementAlreadyUsed = 1003    /*!< SQLite: This cached statement is already being referred to */,
    OrthancPluginErrorCode_SQLiteExecute = 1004    /*!< SQLite: Cannot execute a command */,
    OrthancPluginErrorCode_SQLiteRollbackWithoutTransaction = 1005    /*!< SQLite: Rolling back a nonexistent transaction (have you called Begin()?) */,
    OrthancPluginErrorCode_SQLiteCommitWithoutTransaction = 1006    /*!< SQLite: Committing a nonexistent transaction */,
    OrthancPluginErrorCode_SQLiteRegisterFunction = 1007    /*!< SQLite: Unable to register a function */,
    OrthancPluginErrorCode_SQLiteFlush = 1008    /*!< SQLite: Unable to flush the database */,
    OrthancPluginErrorCode_SQLiteCannotRun = 1009    /*!< SQLite: Cannot run a cached statement */,
    OrthancPluginErrorCode_SQLiteCannotStep = 1010    /*!< SQLite: Cannot step over a cached statement */,
    OrthancPluginErrorCode_SQLiteBindOutOfRange = 1011    /*!< SQLite: Bing a value while out of range (serious error) */,
    OrthancPluginErrorCode_SQLitePrepareStatement = 1012    /*!< SQLite: Cannot prepare a cached statement */,
    OrthancPluginErrorCode_SQLiteTransactionAlreadyStarted = 1013    /*!< SQLite: Beginning the same transaction twice */,
    OrthancPluginErrorCode_SQLiteTransactionCommit = 1014    /*!< SQLite: Failure when committing the transaction */,
    OrthancPluginErrorCode_SQLiteTransactionBegin = 1015    /*!< SQLite: Cannot start a transaction */,
    OrthancPluginErrorCode_DirectoryOverFile = 2000    /*!< The directory to be created is already occupied by a regular file */,
    OrthancPluginErrorCode_FileStorageCannotWrite = 2001    /*!< Unable to create a subdirectory or a file in the file storage */,
    OrthancPluginErrorCode_DirectoryExpected = 2002    /*!< The specified path does not point to a directory */,
    OrthancPluginErrorCode_HttpPortInUse = 2003    /*!< The TCP port of the HTTP server is privileged or already in use */,
    OrthancPluginErrorCode_DicomPortInUse = 2004    /*!< The TCP port of the DICOM server is privileged or already in use */,
    OrthancPluginErrorCode_BadHttpStatusInRest = 2005    /*!< This HTTP status is not allowed in a REST API */,
    OrthancPluginErrorCode_RegularFileExpected = 2006    /*!< The specified path does not point to a regular file */,
    OrthancPluginErrorCode_PathToExecutable = 2007    /*!< Unable to get the path to the executable */,
    OrthancPluginErrorCode_MakeDirectory = 2008    /*!< Cannot create a directory */,
    OrthancPluginErrorCode_BadApplicationEntityTitle = 2009    /*!< An application entity title (AET) cannot be empty or be longer than 16 characters */,
    OrthancPluginErrorCode_NoCFindHandler = 2010    /*!< No request handler factory for DICOM C-FIND SCP */,
    OrthancPluginErrorCode_NoCMoveHandler = 2011    /*!< No request handler factory for DICOM C-MOVE SCP */,
    OrthancPluginErrorCode_NoCStoreHandler = 2012    /*!< No request handler factory for DICOM C-STORE SCP */,
    OrthancPluginErrorCode_NoApplicationEntityFilter = 2013    /*!< No application entity filter */,
    OrthancPluginErrorCode_NoSopClassOrInstance = 2014    /*!< DicomUserConnection: Unable to find the SOP class and instance */,
    OrthancPluginErrorCode_NoPresentationContext = 2015    /*!< DicomUserConnection: No acceptable presentation context for modality */,
    OrthancPluginErrorCode_DicomFindUnavailable = 2016    /*!< DicomUserConnection: The C-FIND command is not supported by the remote SCP */,
    OrthancPluginErrorCode_DicomMoveUnavailable = 2017    /*!< DicomUserConnection: The C-MOVE command is not supported by the remote SCP */,
    OrthancPluginErrorCode_CannotStoreInstance = 2018    /*!< Cannot store an instance */,
    OrthancPluginErrorCode_CreateDicomNotString = 2019    /*!< Only string values are supported when creating DICOM instances */,
    OrthancPluginErrorCode_CreateDicomOverrideTag = 2020    /*!< Trying to override a value inherited from a parent module */,
    OrthancPluginErrorCode_CreateDicomUseContent = 2021    /*!< Use \"Content\" to inject an image into a new DICOM instance */,
    OrthancPluginErrorCode_CreateDicomNoPayload = 2022    /*!< No payload is present for one instance in the series */,
    OrthancPluginErrorCode_CreateDicomUseDataUriScheme = 2023    /*!< The payload of the DICOM instance must be specified according to Data URI scheme */,
    OrthancPluginErrorCode_CreateDicomBadParent = 2024    /*!< Trying to attach a new DICOM instance to an inexistent resource */,
    OrthancPluginErrorCode_CreateDicomParentIsInstance = 2025    /*!< Trying to attach a new DICOM instance to an instance (must be a series, study or patient) */,
    OrthancPluginErrorCode_CreateDicomParentEncoding = 2026    /*!< Unable to get the encoding of the parent resource */,
    OrthancPluginErrorCode_UnknownModality = 2027    /*!< Unknown modality */,
    OrthancPluginErrorCode_BadJobOrdering = 2028    /*!< Bad ordering of filters in a job */,
    OrthancPluginErrorCode_JsonToLuaTable = 2029    /*!< Cannot convert the given JSON object to a Lua table */,
    OrthancPluginErrorCode_CannotCreateLua = 2030    /*!< Cannot create the Lua context */,
    OrthancPluginErrorCode_CannotExecuteLua = 2031    /*!< Cannot execute a Lua command */,
    OrthancPluginErrorCode_LuaAlreadyExecuted = 2032    /*!< Arguments cannot be pushed after the Lua function is executed */,
    OrthancPluginErrorCode_LuaBadOutput = 2033    /*!< The Lua function does not give the expected number of outputs */,
    OrthancPluginErrorCode_NotLuaPredicate = 2034    /*!< The Lua function is not a predicate (only true/false outputs allowed) */,
    OrthancPluginErrorCode_LuaReturnsNoString = 2035    /*!< The Lua function does not return a string */,
    OrthancPluginErrorCode_StorageAreaAlreadyRegistered = 2036    /*!< Another plugin has already registered a custom storage area */,
    OrthancPluginErrorCode_DatabaseBackendAlreadyRegistered = 2037    /*!< Another plugin has already registered a custom database back-end */,
    OrthancPluginErrorCode_DatabaseNotInitialized = 2038    /*!< Plugin trying to call the database during its initialization */,
    OrthancPluginErrorCode_SslDisabled = 2039    /*!< Orthanc has been built without SSL support */,
    OrthancPluginErrorCode_CannotOrderSlices = 2040    /*!< Unable to order the slices of the series */,
    OrthancPluginErrorCode_NoWorklistHandler = 2041    /*!< No request handler factory for DICOM C-Find Modality SCP */,
    OrthancPluginErrorCode_AlreadyExistingTag = 2042    /*!< Cannot override the value of a tag that already exists */,
    OrthancPluginErrorCode_NoStorageCommitmentHandler = 2043    /*!< No request handler factory for DICOM N-ACTION SCP (storage commitment) */,
    OrthancPluginErrorCode_NoCGetHandler = 2044    /*!< No request handler factory for DICOM C-GET SCP */,
    OrthancPluginErrorCode_UnsupportedMediaType = 3000    /*!< Unsupported media type */,

    _OrthancPluginErrorCode_INTERNAL = 0x7fffffff
  } OrthancPluginErrorCode;


  /**
   * Forward declaration of one of the mandatory functions for Orthanc
   * plugins.
   **/
  ORTHANC_PLUGINS_API const char* OrthancPluginGetName();


  /**
   * The various HTTP methods for a REST call.
   **/
  typedef enum
  {
    OrthancPluginHttpMethod_Get = 1,    /*!< GET request */
    OrthancPluginHttpMethod_Post = 2,   /*!< POST request */
    OrthancPluginHttpMethod_Put = 3,    /*!< PUT request */
    OrthancPluginHttpMethod_Delete = 4, /*!< DELETE request */

    _OrthancPluginHttpMethod_INTERNAL = 0x7fffffff
  } OrthancPluginHttpMethod;


  /**
   * @brief The parameters of a REST request.
   * @ingroup Callbacks
   **/
  typedef struct
  {
    /**
     * @brief The HTTP method.
     **/
    OrthancPluginHttpMethod method;

    /**
     * @brief The number of groups of the regular expression.
     **/
    uint32_t                groupsCount;

    /**
     * @brief The matched values for the groups of the regular expression.
     **/
    const char* const*      groups;

    /**
     * @brief For a GET request, the number of GET parameters.
     **/
    uint32_t                getCount;

    /**
     * @brief For a GET request, the keys of the GET parameters.
     **/
    const char* const*      getKeys;

    /**
     * @brief For a GET request, the values of the GET parameters.
     **/
    const char* const*      getValues;

    /**
     * @brief For a PUT or POST request, the content of the body.
     **/
    const void*             body;

    /**
     * @brief For a PUT or POST request, the number of bytes of the body.
     **/
    uint32_t                bodySize;


    /* --------------------------------------------------
       New in version 0.8.1
       -------------------------------------------------- */

    /**
     * @brief The number of HTTP headers.
     **/
    uint32_t                headersCount;

    /**
     * @brief The keys of the HTTP headers (always converted to low-case).
     **/
    const char* const*      headersKeys;

    /**
     * @brief The values of the HTTP headers.
     **/
    const char* const*      headersValues;

  } OrthancPluginHttpRequest;


  typedef enum 
  {
    /* Generic services */
    _OrthancPluginService_LogInfo = 1,
    _OrthancPluginService_LogWarning = 2,
    _OrthancPluginService_LogError = 3,
    _OrthancPluginService_GetOrthancPath = 4,
    _OrthancPluginService_GetOrthancDirectory = 5,
    _OrthancPluginService_GetConfigurationPath = 6,
    _OrthancPluginService_SetPluginProperty = 7,
    _OrthancPluginService_GetGlobalProperty = 8,
    _OrthancPluginService_SetGlobalProperty = 9,
    _OrthancPluginService_GetCommandLineArgumentsCount = 10,
    _OrthancPluginService_GetCommandLineArgument = 11,
    _OrthancPluginService_GetExpectedDatabaseVersion = 12,
    _OrthancPluginService_GetConfiguration = 13,
    _OrthancPluginService_BufferCompression = 14,
    _OrthancPluginService_ReadFile = 15,
    _OrthancPluginService_WriteFile = 16,
    _OrthancPluginService_GetErrorDescription = 17,
    _OrthancPluginService_CallHttpClient = 18,
    _OrthancPluginService_RegisterErrorCode = 19,
    _OrthancPluginService_RegisterDictionaryTag = 20,
    _OrthancPluginService_DicomBufferToJson = 21,
    _OrthancPluginService_DicomInstanceToJson = 22,
    _OrthancPluginService_CreateDicom = 23,
    _OrthancPluginService_ComputeMd5 = 24,
    _OrthancPluginService_ComputeSha1 = 25,
    _OrthancPluginService_LookupDictionary = 26,
    _OrthancPluginService_CallHttpClient2 = 27,
    _OrthancPluginService_GenerateUuid = 28,
    _OrthancPluginService_RegisterPrivateDictionaryTag = 29,
    _OrthancPluginService_AutodetectMimeType = 30,
    _OrthancPluginService_SetMetricsValue = 31,
    _OrthancPluginService_EncodeDicomWebJson = 32,
    _OrthancPluginService_EncodeDicomWebXml = 33,
    _OrthancPluginService_ChunkedHttpClient = 34,    /* New in Orthanc 1.5.7 */
    _OrthancPluginService_GetTagName = 35,           /* New in Orthanc 1.5.7 */
    _OrthancPluginService_EncodeDicomWebJson2 = 36,  /* New in Orthanc 1.7.0 */
    _OrthancPluginService_EncodeDicomWebXml2 = 37,   /* New in Orthanc 1.7.0 */
    _OrthancPluginService_CreateMemoryBuffer = 38,   /* New in Orthanc 1.7.0 */
    _OrthancPluginService_GenerateRestApiAuthorizationToken = 39,   /* New in Orthanc 1.8.1 */
    _OrthancPluginService_CreateMemoryBuffer64 = 40, /* New in Orthanc 1.9.0 */
    _OrthancPluginService_CreateDicom2 = 41,         /* New in Orthanc 1.9.0 */
    _OrthancPluginService_GetDatabaseServerIdentifier = 42,         /* New in Orthanc 1.11.1 */

    /* Registration of callbacks */
    _OrthancPluginService_RegisterRestCallback = 1000,
    _OrthancPluginService_RegisterOnStoredInstanceCallback = 1001,
    _OrthancPluginService_RegisterStorageArea = 1002,
    _OrthancPluginService_RegisterOnChangeCallback = 1003,
    _OrthancPluginService_RegisterRestCallbackNoLock = 1004,
    _OrthancPluginService_RegisterWorklistCallback = 1005,
    _OrthancPluginService_RegisterDecodeImageCallback = 1006,
    _OrthancPluginService_RegisterIncomingHttpRequestFilter = 1007,
    _OrthancPluginService_RegisterFindCallback = 1008,
    _OrthancPluginService_RegisterMoveCallback = 1009,
    _OrthancPluginService_RegisterIncomingHttpRequestFilter2 = 1010,
    _OrthancPluginService_RegisterRefreshMetricsCallback = 1011,
    _OrthancPluginService_RegisterChunkedRestCallback = 1012,  /* New in Orthanc 1.5.7 */
    _OrthancPluginService_RegisterStorageCommitmentScpCallback = 1013,
    _OrthancPluginService_RegisterIncomingDicomInstanceFilter = 1014,
    _OrthancPluginService_RegisterTranscoderCallback = 1015,   /* New in Orthanc 1.7.0 */
    _OrthancPluginService_RegisterStorageArea2 = 1016,         /* New in Orthanc 1.9.0 */
    _OrthancPluginService_RegisterIncomingCStoreInstanceFilter = 1017,  /* New in Orthanc 1.10.0 */
    _OrthancPluginService_RegisterReceivedInstanceCallback = 1018,  /* New in Orthanc 1.10.0 */
    _OrthancPluginService_RegisterWebDavCollection = 1019,     /* New in Orthanc 1.10.1 */

    /* Sending answers to REST calls */
    _OrthancPluginService_AnswerBuffer = 2000,
    _OrthancPluginService_CompressAndAnswerPngImage = 2001,  /* Unused as of Orthanc 0.9.4 */
    _OrthancPluginService_Redirect = 2002,
    _OrthancPluginService_SendHttpStatusCode = 2003,
    _OrthancPluginService_SendUnauthorized = 2004,
    _OrthancPluginService_SendMethodNotAllowed = 2005,
    _OrthancPluginService_SetCookie = 2006,
    _OrthancPluginService_SetHttpHeader = 2007,
    _OrthancPluginService_StartMultipartAnswer = 2008,
    _OrthancPluginService_SendMultipartItem = 2009,
    _OrthancPluginService_SendHttpStatus = 2010,
    _OrthancPluginService_CompressAndAnswerImage = 2011,
    _OrthancPluginService_SendMultipartItem2 = 2012,
    _OrthancPluginService_SetHttpErrorDetails = 2013,

    /* Access to the Orthanc database and API */
    _OrthancPluginService_GetDicomForInstance = 3000,
    _OrthancPluginService_RestApiGet = 3001,
    _OrthancPluginService_RestApiPost = 3002,
    _OrthancPluginService_RestApiDelete = 3003,
    _OrthancPluginService_RestApiPut = 3004,
    _OrthancPluginService_LookupPatient = 3005,
    _OrthancPluginService_LookupStudy = 3006,
    _OrthancPluginService_LookupSeries = 3007,
    _OrthancPluginService_LookupInstance = 3008,
    _OrthancPluginService_LookupStudyWithAccessionNumber = 3009,
    _OrthancPluginService_RestApiGetAfterPlugins = 3010,
    _OrthancPluginService_RestApiPostAfterPlugins = 3011,
    _OrthancPluginService_RestApiDeleteAfterPlugins = 3012,
    _OrthancPluginService_RestApiPutAfterPlugins = 3013,
    _OrthancPluginService_ReconstructMainDicomTags = 3014,
    _OrthancPluginService_RestApiGet2 = 3015,
    _OrthancPluginService_CallRestApi = 3016,              /* New in Orthanc 1.9.2 */

    /* Access to DICOM instances */
    _OrthancPluginService_GetInstanceRemoteAet = 4000,
    _OrthancPluginService_GetInstanceSize = 4001,
    _OrthancPluginService_GetInstanceData = 4002,
    _OrthancPluginService_GetInstanceJson = 4003,
    _OrthancPluginService_GetInstanceSimplifiedJson = 4004,
    _OrthancPluginService_HasInstanceMetadata = 4005,
    _OrthancPluginService_GetInstanceMetadata = 4006,
    _OrthancPluginService_GetInstanceOrigin = 4007,
    _OrthancPluginService_GetInstanceTransferSyntaxUid = 4008,
    _OrthancPluginService_HasInstancePixelData = 4009,
    _OrthancPluginService_CreateDicomInstance = 4010,      /* New in Orthanc 1.7.0 */
    _OrthancPluginService_FreeDicomInstance = 4011,        /* New in Orthanc 1.7.0 */
    _OrthancPluginService_GetInstanceFramesCount = 4012,   /* New in Orthanc 1.7.0 */
    _OrthancPluginService_GetInstanceRawFrame = 4013,      /* New in Orthanc 1.7.0 */
    _OrthancPluginService_GetInstanceDecodedFrame = 4014,  /* New in Orthanc 1.7.0 */
    _OrthancPluginService_TranscodeDicomInstance = 4015,   /* New in Orthanc 1.7.0 */
    _OrthancPluginService_SerializeDicomInstance = 4016,   /* New in Orthanc 1.7.0 */
    _OrthancPluginService_GetInstanceAdvancedJson = 4017,  /* New in Orthanc 1.7.0 */
    _OrthancPluginService_GetInstanceDicomWebJson = 4018,  /* New in Orthanc 1.7.0 */
    _OrthancPluginService_GetInstanceDicomWebXml = 4019,   /* New in Orthanc 1.7.0 */
    
    /* Services for plugins implementing a database back-end */
    _OrthancPluginService_RegisterDatabaseBackend = 5000,    /* New in Orthanc 0.8.6 */
    _OrthancPluginService_DatabaseAnswer = 5001,
    _OrthancPluginService_RegisterDatabaseBackendV2 = 5002,  /* New in Orthanc 0.9.4 */
    _OrthancPluginService_StorageAreaCreate = 5003,
    _OrthancPluginService_StorageAreaRead = 5004,
    _OrthancPluginService_StorageAreaRemove = 5005,
    _OrthancPluginService_RegisterDatabaseBackendV3 = 5006,  /* New in Orthanc 1.9.2 */

    /* Primitives for handling images */
    _OrthancPluginService_GetImagePixelFormat = 6000,
    _OrthancPluginService_GetImageWidth = 6001,
    _OrthancPluginService_GetImageHeight = 6002,
    _OrthancPluginService_GetImagePitch = 6003,
    _OrthancPluginService_GetImageBuffer = 6004,
    _OrthancPluginService_UncompressImage = 6005,
    _OrthancPluginService_FreeImage = 6006,
    _OrthancPluginService_CompressImage = 6007,
    _OrthancPluginService_ConvertPixelFormat = 6008,
    _OrthancPluginService_GetFontsCount = 6009,
    _OrthancPluginService_GetFontInfo = 6010,
    _OrthancPluginService_DrawText = 6011,
    _OrthancPluginService_CreateImage = 6012,
    _OrthancPluginService_CreateImageAccessor = 6013,
    _OrthancPluginService_DecodeDicomImage = 6014,

    /* Primitives for handling C-Find, C-Move and worklists */
    _OrthancPluginService_WorklistAddAnswer = 7000,
    _OrthancPluginService_WorklistMarkIncomplete = 7001,
    _OrthancPluginService_WorklistIsMatch = 7002,
    _OrthancPluginService_WorklistGetDicomQuery = 7003,
    _OrthancPluginService_FindAddAnswer = 7004,
    _OrthancPluginService_FindMarkIncomplete = 7005,
    _OrthancPluginService_GetFindQuerySize = 7006,
    _OrthancPluginService_GetFindQueryTag = 7007,
    _OrthancPluginService_GetFindQueryTagName = 7008,
    _OrthancPluginService_GetFindQueryValue = 7009,
    _OrthancPluginService_CreateFindMatcher = 7010,
    _OrthancPluginService_FreeFindMatcher = 7011,
    _OrthancPluginService_FindMatcherIsMatch = 7012,

    /* Primitives for accessing Orthanc Peers (new in 1.4.2) */
    _OrthancPluginService_GetPeers = 8000,
    _OrthancPluginService_FreePeers = 8001,
    _OrthancPluginService_GetPeersCount = 8003,
    _OrthancPluginService_GetPeerName = 8004,
    _OrthancPluginService_GetPeerUrl = 8005,
    _OrthancPluginService_CallPeerApi = 8006,
    _OrthancPluginService_GetPeerUserProperty = 8007,

    /* Primitives for handling jobs (new in 1.4.2) */
    _OrthancPluginService_CreateJob = 9000,
    _OrthancPluginService_FreeJob = 9001,
    _OrthancPluginService_SubmitJob = 9002,
    _OrthancPluginService_RegisterJobsUnserializer = 9003,
    
    _OrthancPluginService_INTERNAL = 0x7fffffff
  } _OrthancPluginService;


  typedef enum
  {
    _OrthancPluginProperty_Description = 1,
    _OrthancPluginProperty_RootUri = 2,
    _OrthancPluginProperty_OrthancExplorer = 3,

    _OrthancPluginProperty_INTERNAL = 0x7fffffff
  } _OrthancPluginProperty;



  /**
   * The memory layout of the pixels of an image.
   * @ingroup Images
   **/
  typedef enum
  {
    /**
     * @brief Graylevel 8bpp image.
     *
     * The image is graylevel. Each pixel is unsigned and stored in
     * one byte.
     **/
    OrthancPluginPixelFormat_Grayscale8 = 1,

    /**
     * @brief Graylevel, unsigned 16bpp image.
     *
     * The image is graylevel. Each pixel is unsigned and stored in
     * two bytes.
     **/
    OrthancPluginPixelFormat_Grayscale16 = 2,

    /**
     * @brief Graylevel, signed 16bpp image.
     *
     * The image is graylevel. Each pixel is signed and stored in two
     * bytes.
     **/
    OrthancPluginPixelFormat_SignedGrayscale16 = 3,

    /**
     * @brief Color image in RGB24 format.
     *
     * This format describes a color image. The pixels are stored in 3
     * consecutive bytes. The memory layout is RGB.
     **/
    OrthancPluginPixelFormat_RGB24 = 4,

    /**
     * @brief Color image in RGBA32 format.
     *
     * This format describes a color image. The pixels are stored in 4
     * consecutive bytes. The memory layout is RGBA.
     **/
    OrthancPluginPixelFormat_RGBA32 = 5,

    OrthancPluginPixelFormat_Unknown = 6,   /*!< Unknown pixel format */

    /**
     * @brief Color image in RGB48 format.
     *
     * This format describes a color image. The pixels are stored in 6
     * consecutive bytes. The memory layout is RRGGBB.
     **/
    OrthancPluginPixelFormat_RGB48 = 7,

    /**
     * @brief Graylevel, unsigned 32bpp image.
     *
     * The image is graylevel. Each pixel is unsigned and stored in
     * four bytes.
     **/
    OrthancPluginPixelFormat_Grayscale32 = 8,

    /**
     * @brief Graylevel, floating-point 32bpp image.
     *
     * The image is graylevel. Each pixel is floating-point and stored
     * in four bytes.
     **/
    OrthancPluginPixelFormat_Float32 = 9,

    /**
     * @brief Color image in BGRA32 format.
     *
     * This format describes a color image. The pixels are stored in 4
     * consecutive bytes. The memory layout is BGRA.
     **/
    OrthancPluginPixelFormat_BGRA32 = 10,

    /**
     * @brief Graylevel, unsigned 64bpp image.
     *
     * The image is graylevel. Each pixel is unsigned and stored in
     * eight bytes.
     **/
    OrthancPluginPixelFormat_Grayscale64 = 11,

    _OrthancPluginPixelFormat_INTERNAL = 0x7fffffff
  } OrthancPluginPixelFormat;



  /**
   * The content types that are supported by Orthanc plugins.
   **/
  typedef enum
  {
    OrthancPluginContentType_Unknown = 0,             /*!< Unknown content type */
    OrthancPluginContentType_Dicom = 1,               /*!< DICOM */
    OrthancPluginContentType_DicomAsJson = 2,         /*!< JSON summary of a DICOM file */
    OrthancPluginContentType_DicomUntilPixelData = 3, /*!< DICOM Header till pixel data */

    _OrthancPluginContentType_INTERNAL = 0x7fffffff
  } OrthancPluginContentType;



  /**
   * The supported types of DICOM resources.
   **/
  typedef enum
  {
    OrthancPluginResourceType_Patient = 0,     /*!< Patient */
    OrthancPluginResourceType_Study = 1,       /*!< Study */
    OrthancPluginResourceType_Series = 2,      /*!< Series */
    OrthancPluginResourceType_Instance = 3,    /*!< Instance */
    OrthancPluginResourceType_None = 4,        /*!< Unavailable resource type */

    _OrthancPluginResourceType_INTERNAL = 0x7fffffff
  } OrthancPluginResourceType;



  /**
   * The supported types of changes that can be signaled to the change callback.
   * @ingroup Callbacks
   **/
  typedef enum
  {
    OrthancPluginChangeType_CompletedSeries = 0,    /*!< Series is now complete */
    OrthancPluginChangeType_Deleted = 1,            /*!< Deleted resource */
    OrthancPluginChangeType_NewChildInstance = 2,   /*!< A new instance was added to this resource */
    OrthancPluginChangeType_NewInstance = 3,        /*!< New instance received */
    OrthancPluginChangeType_NewPatient = 4,         /*!< New patient created */
    OrthancPluginChangeType_NewSeries = 5,          /*!< New series created */
    OrthancPluginChangeType_NewStudy = 6,           /*!< New study created */
    OrthancPluginChangeType_StablePatient = 7,      /*!< Timeout: No new instance in this patient */
    OrthancPluginChangeType_StableSeries = 8,       /*!< Timeout: No new instance in this series */
    OrthancPluginChangeType_StableStudy = 9,        /*!< Timeout: No new instance in this study */
    OrthancPluginChangeType_OrthancStarted = 10,    /*!< Orthanc has started */
    OrthancPluginChangeType_OrthancStopped = 11,    /*!< Orthanc is stopping */
    OrthancPluginChangeType_UpdatedAttachment = 12, /*!< Some user-defined attachment has changed for this resource */
    OrthancPluginChangeType_UpdatedMetadata = 13,   /*!< Some user-defined metadata has changed for this resource */
    OrthancPluginChangeType_UpdatedPeers = 14,      /*!< The list of Orthanc peers has changed */
    OrthancPluginChangeType_UpdatedModalities = 15, /*!< The list of DICOM modalities has changed */
    OrthancPluginChangeType_JobSubmitted = 16,      /*!< New Job submitted */
    OrthancPluginChangeType_JobSuccess = 17,        /*!< A Job has completed successfully */
    OrthancPluginChangeType_JobFailure = 18,        /*!< A Job has failed */

    _OrthancPluginChangeType_INTERNAL = 0x7fffffff
  } OrthancPluginChangeType;


  /**
   * The compression algorithms that are supported by the Orthanc core.
   * @ingroup Images
   **/
  typedef enum
  {
    OrthancPluginCompressionType_Zlib = 0,          /*!< Standard zlib compression */
    OrthancPluginCompressionType_ZlibWithSize = 1,  /*!< zlib, prefixed with uncompressed size (uint64_t) */
    OrthancPluginCompressionType_Gzip = 2,          /*!< Standard gzip compression */
    OrthancPluginCompressionType_GzipWithSize = 3,  /*!< gzip, prefixed with uncompressed size (uint64_t) */

    _OrthancPluginCompressionType_INTERNAL = 0x7fffffff
  } OrthancPluginCompressionType;


  /**
   * The image formats that are supported by the Orthanc core.
   * @ingroup Images
   **/
  typedef enum
  {
    OrthancPluginImageFormat_Png = 0,    /*!< Image compressed using PNG */
    OrthancPluginImageFormat_Jpeg = 1,   /*!< Image compressed using JPEG */
    OrthancPluginImageFormat_Dicom = 2,  /*!< Image compressed using DICOM */

    _OrthancPluginImageFormat_INTERNAL = 0x7fffffff
  } OrthancPluginImageFormat;


  /**
   * The value representations present in the DICOM standard (version 2013).
   * @ingroup Toolbox
   **/
  typedef enum
  {
    OrthancPluginValueRepresentation_AE = 1,   /*!< Application Entity */
    OrthancPluginValueRepresentation_AS = 2,   /*!< Age String */
    OrthancPluginValueRepresentation_AT = 3,   /*!< Attribute Tag */
    OrthancPluginValueRepresentation_CS = 4,   /*!< Code String */
    OrthancPluginValueRepresentation_DA = 5,   /*!< Date */
    OrthancPluginValueRepresentation_DS = 6,   /*!< Decimal String */
    OrthancPluginValueRepresentation_DT = 7,   /*!< Date Time */
    OrthancPluginValueRepresentation_FD = 8,   /*!< Floating Point Double */
    OrthancPluginValueRepresentation_FL = 9,   /*!< Floating Point Single */
    OrthancPluginValueRepresentation_IS = 10,  /*!< Integer String */
    OrthancPluginValueRepresentation_LO = 11,  /*!< Long String */
    OrthancPluginValueRepresentation_LT = 12,  /*!< Long Text */
    OrthancPluginValueRepresentation_OB = 13,  /*!< Other Byte String */
    OrthancPluginValueRepresentation_OF = 14,  /*!< Other Float String */
    OrthancPluginValueRepresentation_OW = 15,  /*!< Other Word String */
    OrthancPluginValueRepresentation_PN = 16,  /*!< Person Name */
    OrthancPluginValueRepresentation_SH = 17,  /*!< Short String */
    OrthancPluginValueRepresentation_SL = 18,  /*!< Signed Long */
    OrthancPluginValueRepresentation_SQ = 19,  /*!< Sequence of Items */
    OrthancPluginValueRepresentation_SS = 20,  /*!< Signed Short */
    OrthancPluginValueRepresentation_ST = 21,  /*!< Short Text */
    OrthancPluginValueRepresentation_TM = 22,  /*!< Time */
    OrthancPluginValueRepresentation_UI = 23,  /*!< Unique Identifier (UID) */
    OrthancPluginValueRepresentation_UL = 24,  /*!< Unsigned Long */
    OrthancPluginValueRepresentation_UN = 25,  /*!< Unknown */
    OrthancPluginValueRepresentation_US = 26,  /*!< Unsigned Short */
    OrthancPluginValueRepresentation_UT = 27,  /*!< Unlimited Text */

    _OrthancPluginValueRepresentation_INTERNAL = 0x7fffffff
  } OrthancPluginValueRepresentation;


  /**
   * The possible output formats for a DICOM-to-JSON conversion.
   * @ingroup Toolbox
   * @see OrthancPluginDicomToJson()
   **/
  typedef enum
  {
    OrthancPluginDicomToJsonFormat_Full = 1,    /*!< Full output, with most details */
    OrthancPluginDicomToJsonFormat_Short = 2,   /*!< Tags output as hexadecimal numbers */
    OrthancPluginDicomToJsonFormat_Human = 3,   /*!< Human-readable JSON */

    _OrthancPluginDicomToJsonFormat_INTERNAL = 0x7fffffff
  } OrthancPluginDicomToJsonFormat;


  /**
   * Flags to customize a DICOM-to-JSON conversion. By default, binary
   * tags are formatted using Data URI scheme.
   * @ingroup Toolbox
   **/
  typedef enum
  {
    OrthancPluginDicomToJsonFlags_None                  = 0,
    OrthancPluginDicomToJsonFlags_IncludeBinary         = (1 << 0),  /*!< Include the binary tags */
    OrthancPluginDicomToJsonFlags_IncludePrivateTags    = (1 << 1),  /*!< Include the private tags */
    OrthancPluginDicomToJsonFlags_IncludeUnknownTags    = (1 << 2),  /*!< Include the tags unknown by the dictionary */
    OrthancPluginDicomToJsonFlags_IncludePixelData      = (1 << 3),  /*!< Include the pixel data */
    OrthancPluginDicomToJsonFlags_ConvertBinaryToAscii  = (1 << 4),  /*!< Output binary tags as-is, dropping non-ASCII */
    OrthancPluginDicomToJsonFlags_ConvertBinaryToNull   = (1 << 5),  /*!< Signal binary tags as null values */
    OrthancPluginDicomToJsonFlags_StopAfterPixelData    = (1 << 6),  /*!< Stop processing after pixel data (new in 1.9.1) */
    OrthancPluginDicomToJsonFlags_SkipGroupLengths      = (1 << 7),  /*!< Skip tags whose element is zero (new in 1.9.1) */

    _OrthancPluginDicomToJsonFlags_INTERNAL = 0x7fffffff
  } OrthancPluginDicomToJsonFlags;


  /**
   * Flags to the creation of a DICOM file.
   * @ingroup Toolbox
   * @see OrthancPluginCreateDicom()
   **/
  typedef enum
  {
    OrthancPluginCreateDicomFlags_None                  = 0,
    OrthancPluginCreateDicomFlags_DecodeDataUriScheme   = (1 << 0),  /*!< Decode fields encoded using data URI scheme */
    OrthancPluginCreateDicomFlags_GenerateIdentifiers   = (1 << 1),  /*!< Automatically generate DICOM identifiers */

    _OrthancPluginCreateDicomFlags_INTERNAL = 0x7fffffff
  } OrthancPluginCreateDicomFlags;


  /**
   * The constraints on the DICOM identifiers that must be supported
   * by the database plugins.
   * @deprecated Plugins using OrthancPluginConstraintType will be faster
   **/
  typedef enum
  {
    OrthancPluginIdentifierConstraint_Equal = 1,           /*!< Equal */
    OrthancPluginIdentifierConstraint_SmallerOrEqual = 2,  /*!< Less or equal */
    OrthancPluginIdentifierConstraint_GreaterOrEqual = 3,  /*!< More or equal */
    OrthancPluginIdentifierConstraint_Wildcard = 4,        /*!< Case-sensitive wildcard matching (with * and ?) */

    _OrthancPluginIdentifierConstraint_INTERNAL = 0x7fffffff
  } OrthancPluginIdentifierConstraint;


  /**
   * The constraints on the tags (main DICOM tags and identifier tags)
   * that must be supported by the database plugins.
   **/
  typedef enum
  {
    OrthancPluginConstraintType_Equal = 1,           /*!< Equal */
    OrthancPluginConstraintType_SmallerOrEqual = 2,  /*!< Less or equal */
    OrthancPluginConstraintType_GreaterOrEqual = 3,  /*!< More or equal */
    OrthancPluginConstraintType_Wildcard = 4,        /*!< Wildcard matching */
    OrthancPluginConstraintType_List = 5,            /*!< List of values */

    _OrthancPluginConstraintType_INTERNAL = 0x7fffffff
  } OrthancPluginConstraintType;


  /**
   * The origin of a DICOM instance that has been received by Orthanc.
   **/
  typedef enum
  {
    OrthancPluginInstanceOrigin_Unknown = 1,        /*!< Unknown origin */
    OrthancPluginInstanceOrigin_DicomProtocol = 2,  /*!< Instance received through DICOM protocol */
    OrthancPluginInstanceOrigin_RestApi = 3,        /*!< Instance received through REST API of Orthanc */
    OrthancPluginInstanceOrigin_Plugin = 4,         /*!< Instance added to Orthanc by a plugin */
    OrthancPluginInstanceOrigin_Lua = 5,            /*!< Instance added to Orthanc by a Lua script */
    OrthancPluginInstanceOrigin_WebDav = 6,         /*!< Instance received through WebDAV (new in 1.8.0) */

    _OrthancPluginInstanceOrigin_INTERNAL = 0x7fffffff
  } OrthancPluginInstanceOrigin;


  /**
   * The possible status for one single step of a job.
   **/
  typedef enum
  {
    OrthancPluginJobStepStatus_Success = 1,   /*!< The job has successfully executed all its steps */
    OrthancPluginJobStepStatus_Failure = 2,   /*!< The job has failed while executing this step */
    OrthancPluginJobStepStatus_Continue = 3   /*!< The job has still data to process after this step */
  } OrthancPluginJobStepStatus;


  /**
   * Explains why the job should stop and release the resources it has
   * allocated. This is especially important to disambiguate between
   * the "paused" condition and the "final" conditions (success,
   * failure, or canceled).
   **/
  typedef enum
  {
    OrthancPluginJobStopReason_Success = 1,  /*!< The job has succeeded */
    OrthancPluginJobStopReason_Paused = 2,   /*!< The job was paused, and will be resumed later */
    OrthancPluginJobStopReason_Failure = 3,  /*!< The job has failed, and might be resubmitted later */
    OrthancPluginJobStopReason_Canceled = 4  /*!< The job was canceled, and might be resubmitted later */
  } OrthancPluginJobStopReason;


  /**
   * The available types of metrics.
   **/
  typedef enum
  {
    OrthancPluginMetricsType_Default = 0,   /*!< Default metrics */

    /**
     * This metrics represents a time duration. Orthanc will keep the
     * maximum value of the metrics over a sliding window of ten
     * seconds, which is useful if the metrics is sampled frequently.
     **/
    OrthancPluginMetricsType_Timer = 1
  } OrthancPluginMetricsType;
  

  /**
   * The available modes to export a binary DICOM tag into a DICOMweb
   * JSON or XML document.
   **/
  typedef enum
  {
    OrthancPluginDicomWebBinaryMode_Ignore = 0,        /*!< Don't include binary tags */
    OrthancPluginDicomWebBinaryMode_InlineBinary = 1,  /*!< Inline encoding using Base64 */
    OrthancPluginDicomWebBinaryMode_BulkDataUri = 2    /*!< Use a bulk data URI field */
  } OrthancPluginDicomWebBinaryMode;


  /**
   * The available values for the Failure Reason (0008,1197) during
   * storage commitment.
   * http://dicom.nema.org/medical/dicom/2019e/output/chtml/part03/sect_C.14.html#sect_C.14.1.1
   **/
  typedef enum
  {
    OrthancPluginStorageCommitmentFailureReason_Success = 0,
    /*!< Success: The DICOM instance is properly stored in the SCP */

    OrthancPluginStorageCommitmentFailureReason_ProcessingFailure = 1,
    /*!< 0110H: A general failure in processing the operation was encountered */

    OrthancPluginStorageCommitmentFailureReason_NoSuchObjectInstance = 2,
    /*!< 0112H: One or more of the elements in the Referenced SOP
      Instance Sequence was not available */

    OrthancPluginStorageCommitmentFailureReason_ResourceLimitation = 3,
    /*!< 0213H: The SCP does not currently have enough resources to
      store the requested SOP Instance(s) */

    OrthancPluginStorageCommitmentFailureReason_ReferencedSOPClassNotSupported = 4,
    /*!< 0122H: Storage Commitment has been requested for a SOP
      Instance with a SOP Class that is not supported by the SCP */

    OrthancPluginStorageCommitmentFailureReason_ClassInstanceConflict = 5,
    /*!< 0119H: The SOP Class of an element in the Referenced SOP
      Instance Sequence did not correspond to the SOP class registered
      for this SOP Instance at the SCP */

    OrthancPluginStorageCommitmentFailureReason_DuplicateTransactionUID = 6
    /*!< 0131H: The Transaction UID of the Storage Commitment Request
      is already in use */
  } OrthancPluginStorageCommitmentFailureReason;


  /**
   * The action to be taken after ReceivedInstanceCallback is triggered
   **/
  typedef enum
  {
    OrthancPluginReceivedInstanceAction_KeepAsIs = 1, /*!< Keep the instance as is */
    OrthancPluginReceivedInstanceAction_Modify = 2,   /*!< Modify the instance */
    OrthancPluginReceivedInstanceAction_Discard = 3,  /*!< Discard the instance */

    _OrthancPluginReceivedInstanceAction_INTERNAL = 0x7fffffff
  } OrthancPluginReceivedInstanceAction;


  /**
   * @brief A 32-bit memory buffer allocated by the core system of Orthanc.
   *
   * A memory buffer allocated by the core system of Orthanc. When the
   * content of the buffer is not useful anymore, it must be free by a
   * call to ::OrthancPluginFreeMemoryBuffer().
   **/
  typedef struct
  {
    /**
     * @brief The content of the buffer.
     **/
    void*      data;

    /**
     * @brief The number of bytes in the buffer.
     **/
    uint32_t   size;
  } OrthancPluginMemoryBuffer;



  /**
   * @brief A 64-bit memory buffer allocated by the core system of Orthanc.
   *
   * A memory buffer allocated by the core system of Orthanc. When the
   * content of the buffer is not useful anymore, it must be free by a
   * call to ::OrthancPluginFreeMemoryBuffer64().
   **/
  typedef struct
  {
    /**
     * @brief The content of the buffer.
     **/
    void*      data;

    /**
     * @brief The number of bytes in the buffer.
     **/
    uint64_t   size;
  } OrthancPluginMemoryBuffer64;




  /**
   * @brief Opaque structure that represents the HTTP connection to the client application.
   * @ingroup Callbacks
   **/
  typedef struct _OrthancPluginRestOutput_t OrthancPluginRestOutput;



  /**
   * @brief Opaque structure that represents a DICOM instance that is managed by the Orthanc core.
   * @ingroup DicomInstance
   **/
  typedef struct _OrthancPluginDicomInstance_t OrthancPluginDicomInstance;



  /**
   * @brief Opaque structure that represents an image that is uncompressed in memory.
   * @ingroup Images
   **/
  typedef struct _OrthancPluginImage_t OrthancPluginImage;



  /**
   * @brief Opaque structure that represents the storage area that is actually used by Orthanc.
   * @ingroup Images
   **/
  typedef struct _OrthancPluginStorageArea_t OrthancPluginStorageArea;



  /**
   * @brief Opaque structure to an object that represents a C-Find query for worklists.
   * @ingroup DicomCallbacks
   **/
  typedef struct _OrthancPluginWorklistQuery_t OrthancPluginWorklistQuery;



  /**
   * @brief Opaque structure to an object that represents the answers to a C-Find query for worklists.
   * @ingroup DicomCallbacks
   **/
  typedef struct _OrthancPluginWorklistAnswers_t OrthancPluginWorklistAnswers;



  /**
   * @brief Opaque structure to an object that represents a C-Find query.
   * @ingroup DicomCallbacks
   **/
  typedef struct _OrthancPluginFindQuery_t OrthancPluginFindQuery;



  /**
   * @brief Opaque structure to an object that represents the answers to a C-Find query for worklists.
   * @ingroup DicomCallbacks
   **/
  typedef struct _OrthancPluginFindAnswers_t OrthancPluginFindAnswers;



  /**
   * @brief Opaque structure to an object that can be used to check whether a DICOM instance matches a C-Find query.
   * @ingroup Toolbox
   **/
  typedef struct _OrthancPluginFindMatcher_t OrthancPluginFindMatcher;


  
  /**
   * @brief Opaque structure to the set of remote Orthanc Peers that are known to the local Orthanc server.
   * @ingroup Toolbox
   **/
  typedef struct _OrthancPluginPeers_t OrthancPluginPeers;



  /**
   * @brief Opaque structure to a job to be executed by Orthanc.
   * @ingroup Toolbox
   **/
  typedef struct _OrthancPluginJob_t OrthancPluginJob;  



  /**
   * @brief Opaque structure that represents a node in a JSON or XML
   * document used in DICOMweb.
   * @ingroup Toolbox
   **/
  typedef struct _OrthancPluginDicomWebNode_t OrthancPluginDicomWebNode;

  

  /**
   * @brief Signature of a callback function that answers to a REST request.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginRestCallback) (
    OrthancPluginRestOutput* output,
    const char* url,
    const OrthancPluginHttpRequest* request);



  /**
   * @brief Signature of a callback function that is triggered when Orthanc stores a new DICOM instance.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginOnStoredInstanceCallback) (
    const OrthancPluginDicomInstance* instance,
    const char* instanceId);



  /**
   * @brief Signature of a callback function that is triggered when a change happens to some DICOM resource.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginOnChangeCallback) (
    OrthancPluginChangeType changeType,
    OrthancPluginResourceType resourceType,
    const char* resourceId);



  /**
   * @brief Signature of a callback function to decode a DICOM instance as an image.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginDecodeImageCallback) (
    OrthancPluginImage** target,
    const void* dicom,
    const uint32_t size,
    uint32_t frameIndex);



  /**
   * @brief Signature of a function to free dynamic memory.
   * @ingroup Callbacks
   **/
  typedef void (*OrthancPluginFree) (void* buffer);



  /**
   * @brief Signature of a function to set the content of a node
   * encoding a binary DICOM tag, into a JSON or XML document
   * generated for DICOMweb.
   * @ingroup Callbacks
   **/
  typedef void (*OrthancPluginDicomWebSetBinaryNode) (
    OrthancPluginDicomWebNode*       node,
    OrthancPluginDicomWebBinaryMode  mode,
    const char*                      bulkDataUri);
    


  /**
   * @brief Callback for writing to the storage area.
   *
   * Signature of a callback function that is triggered when Orthanc writes a file to the storage area.
   *
   * @param uuid The UUID of the file.
   * @param content The content of the file.
   * @param size The size of the file.
   * @param type The content type corresponding to this file. 
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginStorageCreate) (
    const char* uuid,
    const void* content,
    int64_t size,
    OrthancPluginContentType type);



  /**
   * @brief Callback for reading from the storage area.
   *
   * Signature of a callback function that is triggered when Orthanc reads a file from the storage area.
   *
   * @param content The content of the file (output).
   * @param size The size of the file (output).
   * @param uuid The UUID of the file of interest.
   * @param type The content type corresponding to this file. 
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   * @deprecated New plugins should use OrthancPluginStorageRead2
   * 
   * @warning The "content" buffer *must* have been allocated using
   * the "malloc()" function of your C standard library (i.e. nor
   * "new[]", neither a pointer to a buffer). The "free()" function of
   * your C standard library will automatically be invoked on the
   * "content" pointer.
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginStorageRead) (
    void** content,
    int64_t* size,
    const char* uuid,
    OrthancPluginContentType type);



  /**
   * @brief Callback for reading a whole file from the storage area.
   *
   * Signature of a callback function that is triggered when Orthanc
   * reads a whole file from the storage area.
   *
   * @param target Memory buffer where to store the content of the file. It must be allocated by the
   * plugin using OrthancPluginCreateMemoryBuffer64(). The core of Orthanc will free it.
   * @param uuid The UUID of the file of interest.
   * @param type The content type corresponding to this file. 
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginStorageReadWhole) (
    OrthancPluginMemoryBuffer64* target,
    const char* uuid,
    OrthancPluginContentType type);



  /**
   * @brief Callback for reading a range of a file from the storage area.
   *
   * Signature of a callback function that is triggered when Orthanc
   * reads a portion of a file from the storage area. Orthanc
   * indicates the start position and the length of the range.
   *
   * @param target Memory buffer where to store the content of the range.
   * The memory buffer is allocated and freed by Orthanc. The length of the range
   * of interest corresponds to the size of this buffer.
   * @param uuid The UUID of the file of interest.
   * @param type The content type corresponding to this file. 
   * @param rangeStart Start position of the requested range in the file.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginStorageReadRange) (
    OrthancPluginMemoryBuffer64* target,
    const char* uuid,
    OrthancPluginContentType type,
    uint64_t rangeStart);



  /**
   * @brief Callback for removing a file from the storage area.
   *
   * Signature of a callback function that is triggered when Orthanc deletes a file from the storage area.
   *
   * @param uuid The UUID of the file to be removed.
   * @param type The content type corresponding to this file. 
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginStorageRemove) (
    const char* uuid,
    OrthancPluginContentType type);



  /**
   * @brief Callback to handle the C-Find SCP requests for worklists.
   *
   * Signature of a callback function that is triggered when Orthanc
   * receives a C-Find SCP request against modality worklists.
   *
   * @param answers The target structure where answers must be stored.
   * @param query The worklist query.
   * @param issuerAet The Application Entity Title (AET) of the modality from which the request originates.
   * @param calledAet The Application Entity Title (AET) of the modality that is called by the request.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginWorklistCallback) (
    OrthancPluginWorklistAnswers*     answers,
    const OrthancPluginWorklistQuery* query,
    const char*                       issuerAet,
    const char*                       calledAet);



  /**
   * @brief Callback to filter incoming HTTP requests received by Orthanc.
   *
   * Signature of a callback function that is triggered whenever
   * Orthanc receives an HTTP/REST request, and that answers whether
   * this request should be allowed. If the callback returns "0"
   * ("false"), the server answers with HTTP status code 403
   * (Forbidden).
   *
   * Pay attention to the fact that this function may be invoked
   * concurrently by different threads of the Web server of
   * Orthanc. You must implement proper locking if applicable.
   *
   * @param method The HTTP method used by the request.
   * @param uri The URI of interest.
   * @param ip The IP address of the HTTP client.
   * @param headersCount The number of HTTP headers.
   * @param headersKeys The keys of the HTTP headers (always converted to low-case).
   * @param headersValues The values of the HTTP headers.
   * @return 0 if forbidden access, 1 if allowed access, -1 if error.
   * @ingroup Callbacks
   * @deprecated Please instead use OrthancPluginIncomingHttpRequestFilter2()
   **/
  typedef int32_t (*OrthancPluginIncomingHttpRequestFilter) (
    OrthancPluginHttpMethod  method,
    const char*              uri,
    const char*              ip,
    uint32_t                 headersCount,
    const char* const*       headersKeys,
    const char* const*       headersValues);



  /**
   * @brief Callback to filter incoming HTTP requests received by Orthanc.
   *
   * Signature of a callback function that is triggered whenever
   * Orthanc receives an HTTP/REST request, and that answers whether
   * this request should be allowed. If the callback returns "0"
   * ("false"), the server answers with HTTP status code 403
   * (Forbidden).
   *
   * Pay attention to the fact that this function may be invoked
   * concurrently by different threads of the Web server of
   * Orthanc. You must implement proper locking if applicable.
   *
   * @param method The HTTP method used by the request.
   * @param uri The URI of interest.
   * @param ip The IP address of the HTTP client.
   * @param headersCount The number of HTTP headers.
   * @param headersKeys The keys of the HTTP headers (always converted to low-case).
   * @param headersValues The values of the HTTP headers.
   * @param getArgumentsCount The number of GET arguments (only for the GET HTTP method).
   * @param getArgumentsKeys The keys of the GET arguments (only for the GET HTTP method).
   * @param getArgumentsValues The values of the GET arguments (only for the GET HTTP method).
   * @return 0 if forbidden access, 1 if allowed access, -1 if error.
   * @ingroup Callbacks
   **/
  typedef int32_t (*OrthancPluginIncomingHttpRequestFilter2) (
    OrthancPluginHttpMethod  method,
    const char*              uri,
    const char*              ip,
    uint32_t                 headersCount,
    const char* const*       headersKeys,
    const char* const*       headersValues,
    uint32_t                 getArgumentsCount,
    const char* const*       getArgumentsKeys,
    const char* const*       getArgumentsValues);



  /**
   * @brief Callback to handle incoming C-Find SCP requests.
   *
   * Signature of a callback function that is triggered whenever
   * Orthanc receives a C-Find SCP request not concerning modality
   * worklists.
   *
   * @param answers The target structure where answers must be stored.
   * @param query The worklist query.
   * @param issuerAet The Application Entity Title (AET) of the modality from which the request originates.
   * @param calledAet The Application Entity Title (AET) of the modality that is called by the request.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginFindCallback) (
    OrthancPluginFindAnswers*     answers,
    const OrthancPluginFindQuery* query,
    const char*                   issuerAet,
    const char*                   calledAet);



  /**
   * @brief Callback to handle incoming C-Move SCP requests.
   *
   * Signature of a callback function that is triggered whenever
   * Orthanc receives a C-Move SCP request. The callback receives the
   * type of the resource of interest (study, series, instance...)
   * together with the DICOM tags containing its identifiers. In turn,
   * the plugin must create a driver object that will be responsible
   * for driving the successive move suboperations.
   *
   * @param resourceType The type of the resource of interest. Note
   * that this might be set to ResourceType_None if the
   * QueryRetrieveLevel (0008,0052) tag was not provided by the
   * issuer (i.e. the originator modality).
   * @param patientId Content of the PatientID (0x0010, 0x0020) tag of the resource of interest. Might be NULL.
   * @param accessionNumber Content of the AccessionNumber (0x0008, 0x0050) tag. Might be NULL.
   * @param studyInstanceUid Content of the StudyInstanceUID (0x0020, 0x000d) tag. Might be NULL.
   * @param seriesInstanceUid Content of the SeriesInstanceUID (0x0020, 0x000e) tag. Might be NULL.
   * @param sopInstanceUid Content of the SOPInstanceUID (0x0008, 0x0018) tag. Might be NULL.
   * @param originatorAet The Application Entity Title (AET) of the
   * modality from which the request originates.
   * @param sourceAet The Application Entity Title (AET) of the
   * modality that should send its DICOM files to another modality.
   * @param targetAet The Application Entity Title (AET) of the
   * modality that should receive the DICOM files.
   * @param originatorId The Message ID issued by the originator modality,
   * as found in tag (0000,0110) of the DICOM query emitted by the issuer.
   *
   * @return The NULL value if the plugin cannot deal with this query,
   * or a pointer to the driver object that is responsible for
   * handling the successive move suboperations.
   * 
   * @note If targetAet equals sourceAet, this is actually a query/retrieve operation.
   * @ingroup DicomCallbacks
   **/
  typedef void* (*OrthancPluginMoveCallback) (
    OrthancPluginResourceType  resourceType,
    const char*                patientId,
    const char*                accessionNumber,
    const char*                studyInstanceUid,
    const char*                seriesInstanceUid,
    const char*                sopInstanceUid,
    const char*                originatorAet,
    const char*                sourceAet,
    const char*                targetAet,
    uint16_t                   originatorId);
    

  /**
   * @brief Callback to read the size of a C-Move driver.
   * 
   * Signature of a callback function that returns the number of
   * C-Move suboperations that are to be achieved by the given C-Move
   * driver. This driver is the return value of a previous call to the
   * OrthancPluginMoveCallback() callback.
   *
   * @param moveDriver The C-Move driver of interest.
   * @return The number of suboperations. 
   * @ingroup DicomCallbacks
   **/
  typedef uint32_t (*OrthancPluginGetMoveSize) (void* moveDriver);


  /**
   * @brief Callback to apply one C-Move suboperation.
   * 
   * Signature of a callback function that applies the next C-Move
   * suboperation that os to be achieved by the given C-Move
   * driver. This driver is the return value of a previous call to the
   * OrthancPluginMoveCallback() callback.
   *
   * @param moveDriver The C-Move driver of interest.
   * @return 0 if success, or the error code if failure.
   * @ingroup DicomCallbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginApplyMove) (void* moveDriver);


  /**
   * @brief Callback to free one C-Move driver.
   * 
   * Signature of a callback function that releases the resources
   * allocated by the given C-Move driver. This driver is the return
   * value of a previous call to the OrthancPluginMoveCallback()
   * callback.
   *
   * @param moveDriver The C-Move driver of interest.
   * @ingroup DicomCallbacks
   **/
  typedef void (*OrthancPluginFreeMove) (void* moveDriver);


  /**
   * @brief Callback to finalize one custom job.
   * 
   * Signature of a callback function that releases all the resources
   * allocated by the given job. This job is the argument provided to
   * OrthancPluginCreateJob().
   *
   * @param job The job of interest.
   * @ingroup Toolbox
   **/  
  typedef void (*OrthancPluginJobFinalize) (void* job);


  /**
   * @brief Callback to check the progress of one custom job.
   * 
   * Signature of a callback function that returns the progress of the
   * job.
   *
   * @param job The job of interest.
   * @return The progress, as a floating-point number ranging from 0 to 1.
   * @ingroup Toolbox
   **/  
  typedef float (*OrthancPluginJobGetProgress) (void* job);

  
  /**
   * @brief Callback to retrieve the content of one custom job.
   * 
   * Signature of a callback function that returns human-readable
   * statistics about the job. This statistics must be formatted as a
   * JSON object. This information is notably displayed in the "Jobs"
   * tab of "Orthanc Explorer".
   *
   * @param job The job of interest.
   * @return The statistics, as a JSON object encoded as a string.
   * @ingroup Toolbox
   **/  
  typedef const char* (*OrthancPluginJobGetContent) (void* job);


  /**
   * @brief Callback to serialize one custom job.
   * 
   * Signature of a callback function that returns a serialized
   * version of the job, formatted as a JSON object. This
   * serialization is stored in the Orthanc database, and is used to
   * reload the job on the restart of Orthanc. The "unserialization"
   * callback (with OrthancPluginJobsUnserializer signature) will
   * receive this serialized object.
   *
   * @param job The job of interest.
   * @return The serialized job, as a JSON object encoded as a string.
   * @see OrthancPluginRegisterJobsUnserializer()
   * @ingroup Toolbox
   **/  
  typedef const char* (*OrthancPluginJobGetSerialized) (void* job);


  /**
   * @brief Callback to execute one step of a custom job.
   * 
   * Signature of a callback function that executes one step in the
   * job. The jobs engine of Orthanc will make successive calls to
   * this method, as long as it returns
   * OrthancPluginJobStepStatus_Continue.
   *
   * @param job The job of interest.
   * @return The status of execution.
   * @ingroup Toolbox
   **/  
  typedef OrthancPluginJobStepStatus (*OrthancPluginJobStep) (void* job);


  /**
   * @brief Callback executed once one custom job leaves the "running" state.
   * 
   * Signature of a callback function that is invoked once a job
   * leaves the "running" state. This can happen if the previous call
   * to OrthancPluginJobStep has failed/succeeded, if the host Orthanc
   * server is being stopped, or if the user manually tags the job as
   * paused/canceled. This callback allows the plugin to free
   * resources allocated for running this custom job (e.g. to stop
   * threads, or to remove temporary files).
   * 
   * Note that handling pauses might involves a specific treatment
   * (such a stopping threads, but keeping temporary files on the
   * disk). This "paused" situation can be checked by looking at the
   * "reason" parameter.
   *
   * @param job The job of interest.
   * @param reason The reason for leaving the "running" state. 
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/    
  typedef OrthancPluginErrorCode (*OrthancPluginJobStop) (void* job, 
                                                          OrthancPluginJobStopReason reason);


  /**
   * @brief Callback executed once one stopped custom job is started again.
   * 
   * Signature of a callback function that is invoked once a job
   * leaves the "failure/canceled" state, to be started again. This
   * function will typically reset the progress to zero. Note that
   * before being actually executed, the job would first be tagged as
   * "pending" in the Orthanc jobs engine.
   *
   * @param job The job of interest.
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/    
  typedef OrthancPluginErrorCode (*OrthancPluginJobReset) (void* job);


  /**
   * @brief Callback executed to unserialize a custom job.
   * 
   * Signature of a callback function that unserializes a job that was
   * saved in the Orthanc database.
   *
   * @param jobType The type of the job, as provided to OrthancPluginCreateJob().
   * @param serialized The serialization of the job, as provided by OrthancPluginJobGetSerialized.
   * @return The unserialized job (as created by OrthancPluginCreateJob()), or NULL
   * if this unserializer cannot handle this job type.
   * @see OrthancPluginRegisterJobsUnserializer()
   * @ingroup Callbacks
   **/    
  typedef OrthancPluginJob* (*OrthancPluginJobsUnserializer) (const char* jobType,
                                                              const char* serialized);
  


  /**
   * @brief Callback executed to update the metrics of the plugin.
   * 
   * Signature of a callback function that is called by Orthanc
   * whenever a monitoring tool (such as Prometheus) asks the current
   * values of the metrics. This callback gives the plugin a chance to
   * update its metrics, by calling OrthancPluginSetMetricsValue().
   * This is typically useful for metrics that are expensive to
   * acquire.
   * 
   * @see OrthancPluginRegisterRefreshMetrics()
   * @ingroup Callbacks
   **/
  typedef void (*OrthancPluginRefreshMetricsCallback) ();

  

  /**
   * @brief Callback executed to encode a binary tag in DICOMweb.
   * 
   * Signature of a callback function that is called by Orthanc
   * whenever a DICOM tag that contains a binary value must be written
   * to a JSON or XML node, while a DICOMweb document is being
   * generated. The value representation (VR) of the DICOM tag can be
   * OB, OD, OF, OL, OW, or UN.
   * 
   * @see OrthancPluginEncodeDicomWebJson() and OrthancPluginEncodeDicomWebXml()
   * @param node The node being generated, as provided by Orthanc.
   * @param setter The setter to be used to encode the content of the node. If
   * the setter is not called, the binary tag is not written to the output document.
   * @param levelDepth The depth of the node in the DICOM hierarchy of sequences.
   * This parameter gives the number of elements in the "levelTagGroup", 
   * "levelTagElement", and "levelIndex" arrays.
   * @param levelTagGroup The group of the parent DICOM tags in the hierarchy.
   * @param levelTagElement The element of the parent DICOM tags in the hierarchy.
   * @param levelIndex The index of the node in the parent sequences of the hierarchy.
   * @param tagGroup The group of the DICOM tag of interest.
   * @param tagElement The element of the DICOM tag of interest.
   * @param vr The value representation of the binary DICOM node.
   * @ingroup Callbacks
   **/
  typedef void (*OrthancPluginDicomWebBinaryCallback) (
    OrthancPluginDicomWebNode*          node,
    OrthancPluginDicomWebSetBinaryNode  setter,
    uint32_t                            levelDepth,
    const uint16_t*                     levelTagGroup,
    const uint16_t*                     levelTagElement,
    const uint32_t*                     levelIndex,
    uint16_t                            tagGroup,
    uint16_t                            tagElement,
    OrthancPluginValueRepresentation    vr);



  /**
   * @brief Callback executed to encode a binary tag in DICOMweb.
   * 
   * Signature of a callback function that is called by Orthanc
   * whenever a DICOM tag that contains a binary value must be written
   * to a JSON or XML node, while a DICOMweb document is being
   * generated. The value representation (VR) of the DICOM tag can be
   * OB, OD, OF, OL, OW, or UN.
   * 
   * @see OrthancPluginEncodeDicomWebJson() and OrthancPluginEncodeDicomWebXml()
   * @param node The node being generated, as provided by Orthanc.
   * @param setter The setter to be used to encode the content of the node. If
   * the setter is not called, the binary tag is not written to the output document.
   * @param levelDepth The depth of the node in the DICOM hierarchy of sequences.
   * This parameter gives the number of elements in the "levelTagGroup", 
   * "levelTagElement", and "levelIndex" arrays.
   * @param levelTagGroup The group of the parent DICOM tags in the hierarchy.
   * @param levelTagElement The element of the parent DICOM tags in the hierarchy.
   * @param levelIndex The index of the node in the parent sequences of the hierarchy.
   * @param tagGroup The group of the DICOM tag of interest.
   * @param tagElement The element of the DICOM tag of interest.
   * @param vr The value representation of the binary DICOM node.
   * @param payload The user payload.
   * @ingroup Callbacks
   **/
  typedef void (*OrthancPluginDicomWebBinaryCallback2) (
    OrthancPluginDicomWebNode*          node,
    OrthancPluginDicomWebSetBinaryNode  setter,
    uint32_t                            levelDepth,
    const uint16_t*                     levelTagGroup,
    const uint16_t*                     levelTagElement,
    const uint32_t*                     levelIndex,
    uint16_t                            tagGroup,
    uint16_t                            tagElement,
    OrthancPluginValueRepresentation    vr,
    void*                               payload);



  /**
   * @brief Data structure that contains information about the Orthanc core.
   **/
  typedef struct _OrthancPluginContext_t
  {
    void*                     pluginsManager;
    const char*               orthancVersion;
    OrthancPluginFree         Free;
    OrthancPluginErrorCode  (*InvokeService) (struct _OrthancPluginContext_t* context,
                                              _OrthancPluginService service,
                                              const void* params);
  } OrthancPluginContext;


  
  /**
   * @brief An entry in the dictionary of DICOM tags.
   **/
  typedef struct
  {
    uint16_t                          group;            /*!< The group of the tag */
    uint16_t                          element;          /*!< The element of the tag */
    OrthancPluginValueRepresentation  vr;               /*!< The value representation of the tag */
    uint32_t                          minMultiplicity;  /*!< The minimum multiplicity of the tag */
    uint32_t                          maxMultiplicity;  /*!< The maximum multiplicity of the tag (0 means arbitrary) */
  } OrthancPluginDictionaryEntry;



  /**
   * @brief Free a string.
   * 
   * Free a string that was allocated by the core system of Orthanc.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param str The string to be freed.
   **/
  ORTHANC_PLUGIN_INLINE void  OrthancPluginFreeString(
    OrthancPluginContext* context, 
    char* str)
  {
    if (str != NULL)
    {
      context->Free(str);
    }
  }


  /**
   * @brief Check that the version of the hosting Orthanc is above a given version.
   * 
   * This function checks whether the version of the Orthanc server
   * running this plugin, is above the given version. Contrarily to
   * OrthancPluginCheckVersion(), it is up to the developer of the
   * plugin to make sure that all the Orthanc SDK services called by
   * the plugin are actually implemented in the given version of
   * Orthanc.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param expectedMajor Expected major version.
   * @param expectedMinor Expected minor version.
   * @param expectedRevision Expected revision.
   * @return 1 if and only if the versions are compatible. If the
   * result is 0, the initialization of the plugin should fail.
   * @see OrthancPluginCheckVersion
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE int  OrthancPluginCheckVersionAdvanced(
    OrthancPluginContext* context,
    int expectedMajor,
    int expectedMinor,
    int expectedRevision)
  {
    int major, minor, revision;

    if (sizeof(int32_t) != sizeof(OrthancPluginErrorCode) ||
        sizeof(int32_t) != sizeof(OrthancPluginHttpMethod) ||
        sizeof(int32_t) != sizeof(_OrthancPluginService) ||
        sizeof(int32_t) != sizeof(_OrthancPluginProperty) ||
        sizeof(int32_t) != sizeof(OrthancPluginPixelFormat) ||
        sizeof(int32_t) != sizeof(OrthancPluginContentType) ||
        sizeof(int32_t) != sizeof(OrthancPluginResourceType) ||
        sizeof(int32_t) != sizeof(OrthancPluginChangeType) ||
        sizeof(int32_t) != sizeof(OrthancPluginCompressionType) ||
        sizeof(int32_t) != sizeof(OrthancPluginImageFormat) ||
        sizeof(int32_t) != sizeof(OrthancPluginValueRepresentation) ||
        sizeof(int32_t) != sizeof(OrthancPluginDicomToJsonFormat) ||
        sizeof(int32_t) != sizeof(OrthancPluginDicomToJsonFlags) ||
        sizeof(int32_t) != sizeof(OrthancPluginCreateDicomFlags) ||
        sizeof(int32_t) != sizeof(OrthancPluginIdentifierConstraint) ||
        sizeof(int32_t) != sizeof(OrthancPluginInstanceOrigin) ||
        sizeof(int32_t) != sizeof(OrthancPluginJobStepStatus) ||
        sizeof(int32_t) != sizeof(OrthancPluginConstraintType) ||
        sizeof(int32_t) != sizeof(OrthancPluginMetricsType) ||
        sizeof(int32_t) != sizeof(OrthancPluginDicomWebBinaryMode) ||
        sizeof(int32_t) != sizeof(OrthancPluginStorageCommitmentFailureReason) ||
        sizeof(int32_t) != sizeof(OrthancPluginReceivedInstanceAction))
    {
      /* Mismatch in the size of the enumerations */
      return 0;
    }

    /* Assume compatibility with the mainline */
    if (!strcmp(context->orthancVersion, "mainline"))
    {
      return 1;
    }

    /* Parse the version of the Orthanc core */
    if ( 
#ifdef _MSC_VER
      sscanf_s
#else
      sscanf
#endif
      (context->orthancVersion, "%4d.%4d.%4d", &major, &minor, &revision) != 3)
    {
      return 0;
    }

    /* Check the major number of the version */

    if (major > expectedMajor)
    {
      return 1;
    }

    if (major < expectedMajor)
    {
      return 0;
    }

    /* Check the minor number of the version */

    if (minor > expectedMinor)
    {
      return 1;
    }

    if (minor < expectedMinor)
    {
      return 0;
    }

    /* Check the revision number of the version */

    if (revision >= expectedRevision)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }


  /**
   * @brief Check the compatibility of the plugin wrt. the version of its hosting Orthanc.
   * 
   * This function checks whether the version of the Orthanc server
   * running this plugin, is above the version of the current Orthanc
   * SDK header. This guarantees that the plugin is compatible with
   * the hosting Orthanc (i.e. it will not call unavailable services).
   * The result of this function should always be checked in the
   * OrthancPluginInitialize() entry point of the plugin.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return 1 if and only if the versions are compatible. If the
   * result is 0, the initialization of the plugin should fail.
   * @see OrthancPluginCheckVersionAdvanced
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE int  OrthancPluginCheckVersion(
    OrthancPluginContext* context)
  {
    return OrthancPluginCheckVersionAdvanced(
      context,
      ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER,
      ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER,
      ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER);
  }


  /**
   * @brief Free a memory buffer.
   * 
   * Free a memory buffer that was allocated by the core system of Orthanc.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param buffer The memory buffer to release.
   **/
  ORTHANC_PLUGIN_INLINE void  OrthancPluginFreeMemoryBuffer(
    OrthancPluginContext* context, 
    OrthancPluginMemoryBuffer* buffer)
  {
    context->Free(buffer->data);
  }


  /**
   * @brief Free a memory buffer.
   * 
   * Free a memory buffer that was allocated by the core system of Orthanc.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param buffer The memory buffer to release.
   **/
  ORTHANC_PLUGIN_INLINE void  OrthancPluginFreeMemoryBuffer64(
    OrthancPluginContext* context, 
    OrthancPluginMemoryBuffer64* buffer)
  {
    context->Free(buffer->data);
  }


  /**
   * @brief Log an error.
   *
   * Log an error message using the Orthanc logging system.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param message The message to be logged.
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginLogError(
    OrthancPluginContext* context,
    const char* message)
  {
    context->InvokeService(context, _OrthancPluginService_LogError, message);
  }


  /**
   * @brief Log a warning.
   *
   * Log a warning message using the Orthanc logging system.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param message The message to be logged.
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginLogWarning(
    OrthancPluginContext* context,
    const char* message)
  {
    context->InvokeService(context, _OrthancPluginService_LogWarning, message);
  }


  /**
   * @brief Log an information.
   *
   * Log an information message using the Orthanc logging system.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param message The message to be logged.
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginLogInfo(
    OrthancPluginContext* context,
    const char* message)
  {
    context->InvokeService(context, _OrthancPluginService_LogInfo, message);
  }



  typedef struct
  {
    const char* pathRegularExpression;
    OrthancPluginRestCallback callback;
  } _OrthancPluginRestCallback;

  /**
   * @brief Register a REST callback.
   *
   * This function registers a REST callback against a regular
   * expression for a URI. This function must be called during the
   * initialization of the plugin, i.e. inside the
   * OrthancPluginInitialize() public function.
   *
   * Each REST callback is guaranteed to run in mutual exclusion.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param pathRegularExpression Regular expression for the URI. May contain groups. 
   * @param callback The callback function to handle the REST call.
   * @see OrthancPluginRegisterRestCallbackNoLock()
   *
   * @note
   * The regular expression is case sensitive and must follow the
   * [Perl syntax](https://www.boost.org/doc/libs/1_67_0/libs/regex/doc/html/boost_regex/syntax/perl_syntax.html).
   *
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginRegisterRestCallback(
    OrthancPluginContext*     context,
    const char*               pathRegularExpression,
    OrthancPluginRestCallback callback)
  {
    _OrthancPluginRestCallback params;
    params.pathRegularExpression = pathRegularExpression;
    params.callback = callback;
    context->InvokeService(context, _OrthancPluginService_RegisterRestCallback, &params);
  }



  /**
   * @brief Register a REST callback, without locking.
   *
   * This function registers a REST callback against a regular
   * expression for a URI. This function must be called during the
   * initialization of the plugin, i.e. inside the
   * OrthancPluginInitialize() public function.
   *
   * Contrarily to OrthancPluginRegisterRestCallback(), the callback
   * will NOT be invoked in mutual exclusion. This can be useful for
   * high-performance plugins that must handle concurrent requests
   * (Orthanc uses a pool of threads, one thread being assigned to
   * each incoming HTTP request). Of course, if using this function,
   * it is up to the plugin to implement the required locking
   * mechanisms.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param pathRegularExpression Regular expression for the URI. May contain groups.
   * @param callback The callback function to handle the REST call.
   * @see OrthancPluginRegisterRestCallback()
   *
   * @note
   * The regular expression is case sensitive and must follow the
   * [Perl syntax](https://www.boost.org/doc/libs/1_67_0/libs/regex/doc/html/boost_regex/syntax/perl_syntax.html).
   *
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginRegisterRestCallbackNoLock(
    OrthancPluginContext*     context,
    const char*               pathRegularExpression,
    OrthancPluginRestCallback callback)
  {
    _OrthancPluginRestCallback params;
    params.pathRegularExpression = pathRegularExpression;
    params.callback = callback;
    context->InvokeService(context, _OrthancPluginService_RegisterRestCallbackNoLock, &params);
  }



  typedef struct
  {
    OrthancPluginOnStoredInstanceCallback callback;
  } _OrthancPluginOnStoredInstanceCallback;

  /**
   * @brief Register a callback for received instances.
   *
   * This function registers a callback function that is called
   * whenever a new DICOM instance is stored into the Orthanc core.
   *
   * @warning Your callback function will be called synchronously with
   * the core of Orthanc. This implies that deadlocks might emerge if
   * you call other core primitives of Orthanc in your callback (such
   * deadlocks are particularly visible in the presence of other plugins
   * or Lua scripts). It is thus strongly advised to avoid any call to
   * the REST API of Orthanc in the callback. If you have to call
   * other primitives of Orthanc, you should make these calls in a
   * separate thread, passing the pending events to be processed
   * through a message queue.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback function.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginRegisterOnStoredInstanceCallback(
    OrthancPluginContext*                  context,
    OrthancPluginOnStoredInstanceCallback  callback)
  {
    _OrthancPluginOnStoredInstanceCallback params;
    params.callback = callback;

    context->InvokeService(context, _OrthancPluginService_RegisterOnStoredInstanceCallback, &params);
  }



  typedef struct
  {
    OrthancPluginRestOutput* output;
    const void*              answer;
    uint32_t                 answerSize;
    const char*              mimeType;
  } _OrthancPluginAnswerBuffer;

  /**
   * @brief Answer to a REST request.
   *
   * This function answers to a REST request with the content of a memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param answer Pointer to the memory buffer containing the answer.
   * @param answerSize Number of bytes of the answer.
   * @param mimeType The MIME type of the answer.
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginAnswerBuffer(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    const void*              answer,
    uint32_t                 answerSize,
    const char*              mimeType)
  {
    _OrthancPluginAnswerBuffer params;
    params.output = output;
    params.answer = answer;
    params.answerSize = answerSize;
    params.mimeType = mimeType;
    context->InvokeService(context, _OrthancPluginService_AnswerBuffer, &params);
  }


  typedef struct
  {
    OrthancPluginRestOutput*  output;
    OrthancPluginPixelFormat  format;
    uint32_t                  width;
    uint32_t                  height;
    uint32_t                  pitch;
    const void*               buffer;
  } _OrthancPluginCompressAndAnswerPngImage;

  typedef struct
  {
    OrthancPluginRestOutput*  output;
    OrthancPluginImageFormat  imageFormat;
    OrthancPluginPixelFormat  pixelFormat;
    uint32_t                  width;
    uint32_t                  height;
    uint32_t                  pitch;
    const void*               buffer;
    uint8_t                   quality;
  } _OrthancPluginCompressAndAnswerImage;


  /**
   * @brief Answer to a REST request with a PNG image.
   *
   * This function answers to a REST request with a PNG image. The
   * parameters of this function describe a memory buffer that
   * contains an uncompressed image. The image will be automatically compressed
   * as a PNG image by the core system of Orthanc.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param format The memory layout of the uncompressed image.
   * @param width The width of the image.
   * @param height The height of the image.
   * @param pitch The pitch of the image (i.e. the number of bytes
   * between 2 successive lines of the image in the memory buffer).
   * @param buffer The memory buffer containing the uncompressed image.
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginCompressAndAnswerPngImage(
    OrthancPluginContext*     context,
    OrthancPluginRestOutput*  output,
    OrthancPluginPixelFormat  format,
    uint32_t                  width,
    uint32_t                  height,
    uint32_t                  pitch,
    const void*               buffer)
  {
    _OrthancPluginCompressAndAnswerImage params;
    params.output = output;
    params.imageFormat = OrthancPluginImageFormat_Png;
    params.pixelFormat = format;
    params.width = width;
    params.height = height;
    params.pitch = pitch;
    params.buffer = buffer;
    params.quality = 0;  /* No quality for PNG */
    context->InvokeService(context, _OrthancPluginService_CompressAndAnswerImage, &params);
  }



  typedef struct
  {
    OrthancPluginMemoryBuffer*  target;
    const char*                 instanceId;
  } _OrthancPluginGetDicomForInstance;

  /**
   * @brief Retrieve a DICOM instance using its Orthanc identifier.
   * 
   * Retrieve a DICOM instance using its Orthanc identifier. The DICOM
   * file is stored into a newly allocated memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param instanceId The Orthanc identifier of the DICOM instance of interest.
   * @return 0 if success, or the error code if failure.
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginGetDicomForInstance(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 instanceId)
  {
    _OrthancPluginGetDicomForInstance params;
    params.target = target;
    params.instanceId = instanceId;
    return context->InvokeService(context, _OrthancPluginService_GetDicomForInstance, &params);
  }



  typedef struct
  {
    OrthancPluginMemoryBuffer*  target;
    const char*                 uri;
  } _OrthancPluginRestApiGet;

  /**
   * @brief Make a GET call to the built-in Orthanc REST API.
   * 
   * Make a GET call to the built-in Orthanc REST API. The result to
   * the query is stored into a newly allocated memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param uri The URI in the built-in Orthanc API.
   * @return 0 if success, or the error code if failure.
   * @note If the resource is not existing (error 404), the error code will be OrthancPluginErrorCode_UnknownResource.
   * @see OrthancPluginRestApiGetAfterPlugins
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRestApiGet(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 uri)
  {
    _OrthancPluginRestApiGet params;
    params.target = target;
    params.uri = uri;
    return context->InvokeService(context, _OrthancPluginService_RestApiGet, &params);
  }



  /**
   * @brief Make a GET call to the REST API, as tainted by the plugins.
   * 
   * Make a GET call to the Orthanc REST API, after all the plugins
   * are applied. In other words, if some plugin overrides or adds the
   * called URI to the built-in Orthanc REST API, this call will
   * return the result provided by this plugin. The result to the
   * query is stored into a newly allocated memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param uri The URI in the built-in Orthanc API.
   * @return 0 if success, or the error code if failure.
   * @note If the resource is not existing (error 404), the error code will be OrthancPluginErrorCode_UnknownResource.
   * @see OrthancPluginRestApiGet
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRestApiGetAfterPlugins(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 uri)
  {
    _OrthancPluginRestApiGet params;
    params.target = target;
    params.uri = uri;
    return context->InvokeService(context, _OrthancPluginService_RestApiGetAfterPlugins, &params);
  }



  typedef struct
  {
    OrthancPluginMemoryBuffer*  target;
    const char*                 uri;
    const void*                 body;
    uint32_t                    bodySize;
  } _OrthancPluginRestApiPostPut;

  /**
   * @brief Make a POST call to the built-in Orthanc REST API.
   * 
   * Make a POST call to the built-in Orthanc REST API. The result to
   * the query is stored into a newly allocated memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param uri The URI in the built-in Orthanc API.
   * @param body The body of the POST request.
   * @param bodySize The size of the body.
   * @return 0 if success, or the error code if failure.
   * @note If the resource is not existing (error 404), the error code will be OrthancPluginErrorCode_UnknownResource.
   * @see OrthancPluginRestApiPostAfterPlugins
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRestApiPost(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 uri,
    const void*                 body,
    uint32_t                    bodySize)
  {
    _OrthancPluginRestApiPostPut params;
    params.target = target;
    params.uri = uri;
    params.body = body;
    params.bodySize = bodySize;
    return context->InvokeService(context, _OrthancPluginService_RestApiPost, &params);
  }


  /**
   * @brief Make a POST call to the REST API, as tainted by the plugins.
   * 
   * Make a POST call to the Orthanc REST API, after all the plugins
   * are applied. In other words, if some plugin overrides or adds the
   * called URI to the built-in Orthanc REST API, this call will
   * return the result provided by this plugin. The result to the
   * query is stored into a newly allocated memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param uri The URI in the built-in Orthanc API.
   * @param body The body of the POST request.
   * @param bodySize The size of the body.
   * @return 0 if success, or the error code if failure.
   * @note If the resource is not existing (error 404), the error code will be OrthancPluginErrorCode_UnknownResource.
   * @see OrthancPluginRestApiPost
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRestApiPostAfterPlugins(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 uri,
    const void*                 body,
    uint32_t                    bodySize)
  {
    _OrthancPluginRestApiPostPut params;
    params.target = target;
    params.uri = uri;
    params.body = body;
    params.bodySize = bodySize;
    return context->InvokeService(context, _OrthancPluginService_RestApiPostAfterPlugins, &params);
  }



  /**
   * @brief Make a DELETE call to the built-in Orthanc REST API.
   * 
   * Make a DELETE call to the built-in Orthanc REST API.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param uri The URI to delete in the built-in Orthanc API.
   * @return 0 if success, or the error code if failure.
   * @note If the resource is not existing (error 404), the error code will be OrthancPluginErrorCode_UnknownResource.
   * @see OrthancPluginRestApiDeleteAfterPlugins
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRestApiDelete(
    OrthancPluginContext*       context,
    const char*                 uri)
  {
    return context->InvokeService(context, _OrthancPluginService_RestApiDelete, uri);
  }


  /**
   * @brief Make a DELETE call to the REST API, as tainted by the plugins.
   * 
   * Make a DELETE call to the Orthanc REST API, after all the plugins
   * are applied. In other words, if some plugin overrides or adds the
   * called URI to the built-in Orthanc REST API, this call will
   * return the result provided by this plugin. 
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param uri The URI to delete in the built-in Orthanc API.
   * @return 0 if success, or the error code if failure.
   * @note If the resource is not existing (error 404), the error code will be OrthancPluginErrorCode_UnknownResource.
   * @see OrthancPluginRestApiDelete
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRestApiDeleteAfterPlugins(
    OrthancPluginContext*       context,
    const char*                 uri)
  {
    return context->InvokeService(context, _OrthancPluginService_RestApiDeleteAfterPlugins, uri);
  }



  /**
   * @brief Make a PUT call to the built-in Orthanc REST API.
   * 
   * Make a PUT call to the built-in Orthanc REST API. The result to
   * the query is stored into a newly allocated memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param uri The URI in the built-in Orthanc API.
   * @param body The body of the PUT request.
   * @param bodySize The size of the body.
   * @return 0 if success, or the error code if failure.
   * @note If the resource is not existing (error 404), the error code will be OrthancPluginErrorCode_UnknownResource.
   * @see OrthancPluginRestApiPutAfterPlugins
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRestApiPut(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 uri,
    const void*                 body,
    uint32_t                    bodySize)
  {
    _OrthancPluginRestApiPostPut params;
    params.target = target;
    params.uri = uri;
    params.body = body;
    params.bodySize = bodySize;
    return context->InvokeService(context, _OrthancPluginService_RestApiPut, &params);
  }



  /**
   * @brief Make a PUT call to the REST API, as tainted by the plugins.
   * 
   * Make a PUT call to the Orthanc REST API, after all the plugins
   * are applied. In other words, if some plugin overrides or adds the
   * called URI to the built-in Orthanc REST API, this call will
   * return the result provided by this plugin. The result to the
   * query is stored into a newly allocated memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param uri The URI in the built-in Orthanc API.
   * @param body The body of the PUT request.
   * @param bodySize The size of the body.
   * @return 0 if success, or the error code if failure.
   * @note If the resource is not existing (error 404), the error code will be OrthancPluginErrorCode_UnknownResource.
   * @see OrthancPluginRestApiPut
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRestApiPutAfterPlugins(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 uri,
    const void*                 body,
    uint32_t                    bodySize)
  {
    _OrthancPluginRestApiPostPut params;
    params.target = target;
    params.uri = uri;
    params.body = body;
    params.bodySize = bodySize;
    return context->InvokeService(context, _OrthancPluginService_RestApiPutAfterPlugins, &params);
  }



  typedef struct
  {
    OrthancPluginRestOutput* output;
    const char*              argument;
  } _OrthancPluginOutputPlusArgument;

  /**
   * @brief Redirect a REST request.
   *
   * This function answers to a REST request by redirecting the user
   * to another URI using HTTP status 301.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param redirection Where to redirect.
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginRedirect(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    const char*              redirection)
  {
    _OrthancPluginOutputPlusArgument params;
    params.output = output;
    params.argument = redirection;
    context->InvokeService(context, _OrthancPluginService_Redirect, &params);
  }



  typedef struct
  {
    char**       result;
    const char*  argument;
  } _OrthancPluginRetrieveDynamicString;

  /**
   * @brief Look for a patient.
   *
   * Look for a patient stored in Orthanc, using its Patient ID tag (0x0010, 0x0020).
   * This function uses the database index to run as fast as possible (it does not loop
   * over all the stored patients).
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param patientID The Patient ID of interest.
   * @return The NULL value if the patient is non-existent, or a string containing the 
   * Orthanc ID of the patient. This string must be freed by OrthancPluginFreeString().
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginLookupPatient(
    OrthancPluginContext*  context,
    const char*            patientID)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = patientID;

    if (context->InvokeService(context, _OrthancPluginService_LookupPatient, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Look for a study.
   *
   * Look for a study stored in Orthanc, using its Study Instance UID tag (0x0020, 0x000d).
   * This function uses the database index to run as fast as possible (it does not loop
   * over all the stored studies).
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param studyUID The Study Instance UID of interest.
   * @return The NULL value if the study is non-existent, or a string containing the 
   * Orthanc ID of the study. This string must be freed by OrthancPluginFreeString().
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginLookupStudy(
    OrthancPluginContext*  context,
    const char*            studyUID)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = studyUID;

    if (context->InvokeService(context, _OrthancPluginService_LookupStudy, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Look for a study, using the accession number.
   *
   * Look for a study stored in Orthanc, using its Accession Number tag (0x0008, 0x0050).
   * This function uses the database index to run as fast as possible (it does not loop
   * over all the stored studies).
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param accessionNumber The Accession Number of interest.
   * @return The NULL value if the study is non-existent, or a string containing the 
   * Orthanc ID of the study. This string must be freed by OrthancPluginFreeString().
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginLookupStudyWithAccessionNumber(
    OrthancPluginContext*  context,
    const char*            accessionNumber)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = accessionNumber;

    if (context->InvokeService(context, _OrthancPluginService_LookupStudyWithAccessionNumber, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Look for a series.
   *
   * Look for a series stored in Orthanc, using its Series Instance UID tag (0x0020, 0x000e).
   * This function uses the database index to run as fast as possible (it does not loop
   * over all the stored series).
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param seriesUID The Series Instance UID of interest.
   * @return The NULL value if the series is non-existent, or a string containing the 
   * Orthanc ID of the series. This string must be freed by OrthancPluginFreeString().
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginLookupSeries(
    OrthancPluginContext*  context,
    const char*            seriesUID)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = seriesUID;

    if (context->InvokeService(context, _OrthancPluginService_LookupSeries, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Look for an instance.
   *
   * Look for an instance stored in Orthanc, using its SOP Instance UID tag (0x0008, 0x0018).
   * This function uses the database index to run as fast as possible (it does not loop
   * over all the stored instances).
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param sopInstanceUID The SOP Instance UID of interest.
   * @return The NULL value if the instance is non-existent, or a string containing the 
   * Orthanc ID of the instance. This string must be freed by OrthancPluginFreeString().
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginLookupInstance(
    OrthancPluginContext*  context,
    const char*            sopInstanceUID)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = sopInstanceUID;

    if (context->InvokeService(context, _OrthancPluginService_LookupInstance, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }



  typedef struct
  {
    OrthancPluginRestOutput* output;
    uint16_t                 status;
  } _OrthancPluginSendHttpStatusCode;

  /**
   * @brief Send a HTTP status code.
   *
   * This function answers to a REST request by sending a HTTP status
   * code (such as "400 - Bad Request"). Note that:
   * - Successful requests (status 200) must use ::OrthancPluginAnswerBuffer().
   * - Redirections (status 301) must use ::OrthancPluginRedirect().
   * - Unauthorized access (status 401) must use ::OrthancPluginSendUnauthorized().
   * - Methods not allowed (status 405) must use ::OrthancPluginSendMethodNotAllowed().
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param status The HTTP status code to be sent.
   * @ingroup REST
   * @see OrthancPluginSendHttpStatus()
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginSendHttpStatusCode(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    uint16_t                 status)
  {
    _OrthancPluginSendHttpStatusCode params;
    params.output = output;
    params.status = status;
    context->InvokeService(context, _OrthancPluginService_SendHttpStatusCode, &params);
  }


  /**
   * @brief Signal that a REST request is not authorized.
   *
   * This function answers to a REST request by signaling that it is
   * not authorized.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param realm The realm for the authorization process.
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginSendUnauthorized(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    const char*              realm)
  {
    _OrthancPluginOutputPlusArgument params;
    params.output = output;
    params.argument = realm;
    context->InvokeService(context, _OrthancPluginService_SendUnauthorized, &params);
  }


  /**
   * @brief Signal that this URI does not support this HTTP method.
   *
   * This function answers to a REST request by signaling that the
   * queried URI does not support this method.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param allowedMethods The allowed methods for this URI (e.g. "GET,POST" after a PUT or a POST request).
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginSendMethodNotAllowed(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    const char*              allowedMethods)
  {
    _OrthancPluginOutputPlusArgument params;
    params.output = output;
    params.argument = allowedMethods;
    context->InvokeService(context, _OrthancPluginService_SendMethodNotAllowed, &params);
  }


  typedef struct
  {
    OrthancPluginRestOutput* output;
    const char*              key;
    const char*              value;
  } _OrthancPluginSetHttpHeader;

  /**
   * @brief Set a cookie.
   *
   * This function sets a cookie in the HTTP client.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param cookie The cookie to be set.
   * @param value The value of the cookie.
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginSetCookie(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    const char*              cookie,
    const char*              value)
  {
    _OrthancPluginSetHttpHeader params;
    params.output = output;
    params.key = cookie;
    params.value = value;
    context->InvokeService(context, _OrthancPluginService_SetCookie, &params);
  }


  /**
   * @brief Set some HTTP header.
   *
   * This function sets a HTTP header in the HTTP answer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param key The HTTP header to be set.
   * @param value The value of the HTTP header.
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginSetHttpHeader(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    const char*              key,
    const char*              value)
  {
    _OrthancPluginSetHttpHeader params;
    params.output = output;
    params.key = key;
    params.value = value;
    context->InvokeService(context, _OrthancPluginService_SetHttpHeader, &params);
  }


  typedef struct
  {
    char**                             resultStringToFree;
    const char**                       resultString;
    int64_t*                           resultInt64;
    const char*                        key;
    const OrthancPluginDicomInstance*  instance;
    OrthancPluginInstanceOrigin*       resultOrigin;   /* New in Orthanc 0.9.5 SDK */
  } _OrthancPluginAccessDicomInstance;


  /**
   * @brief Get the AET of a DICOM instance.
   *
   * This function returns the Application Entity Title (AET) of the
   * DICOM modality from which a DICOM instance originates.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @return The AET if success, NULL if error.
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE const char* OrthancPluginGetInstanceRemoteAet(
    OrthancPluginContext*              context,
    const OrthancPluginDicomInstance*  instance)
  {
    const char* result;

    _OrthancPluginAccessDicomInstance params;
    memset(&params, 0, sizeof(params));
    params.resultString = &result;
    params.instance = instance;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceRemoteAet, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Get the size of a DICOM file.
   *
   * This function returns the number of bytes of the given DICOM instance.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @return The size of the file, -1 in case of error.
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE int64_t OrthancPluginGetInstanceSize(
    OrthancPluginContext*             context,
    const OrthancPluginDicomInstance* instance)
  {
    int64_t size;

    _OrthancPluginAccessDicomInstance params;
    memset(&params, 0, sizeof(params));
    params.resultInt64 = &size;
    params.instance = instance;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceSize, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return -1;
    }
    else
    {
      return size;
    }
  }


  /**
   * @brief Get the data of a DICOM file.
   *
   * This function returns a pointer to the content of the given DICOM instance.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @return The pointer to the DICOM data, NULL in case of error.
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE const void* OrthancPluginGetInstanceData(
    OrthancPluginContext*              context,
    const OrthancPluginDicomInstance*  instance)
  {
    const char* result;

    _OrthancPluginAccessDicomInstance params;
    memset(&params, 0, sizeof(params));
    params.resultString = &result;
    params.instance = instance;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceData, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Get the DICOM tag hierarchy as a JSON file.
   *
   * This function returns a pointer to a newly created string
   * containing a JSON file. This JSON file encodes the tag hierarchy
   * of the given DICOM instance.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @return The NULL value in case of error, or a string containing the JSON file.
   * This string must be freed by OrthancPluginFreeString().
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGetInstanceJson(
    OrthancPluginContext*              context,
    const OrthancPluginDicomInstance*  instance)
  {
    char* result;

    _OrthancPluginAccessDicomInstance params;
    memset(&params, 0, sizeof(params));
    params.resultStringToFree = &result;
    params.instance = instance;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceJson, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Get the DICOM tag hierarchy as a JSON file (with simplification).
   *
   * This function returns a pointer to a newly created string
   * containing a JSON file. This JSON file encodes the tag hierarchy
   * of the given DICOM instance. In contrast with
   * ::OrthancPluginGetInstanceJson(), the returned JSON file is in
   * its simplified version.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @return The NULL value in case of error, or a string containing the JSON file.
   * This string must be freed by OrthancPluginFreeString().
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGetInstanceSimplifiedJson(
    OrthancPluginContext*              context,
    const OrthancPluginDicomInstance*  instance)
  {
    char* result;

    _OrthancPluginAccessDicomInstance params;
    memset(&params, 0, sizeof(params));
    params.resultStringToFree = &result;
    params.instance = instance;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceSimplifiedJson, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Check whether a DICOM instance is associated with some metadata.
   *
   * This function checks whether the DICOM instance of interest is
   * associated with some metadata. As of Orthanc 0.8.1, in the
   * callbacks registered by
   * ::OrthancPluginRegisterOnStoredInstanceCallback(), the only
   * possibly available metadata are "ReceptionDate", "RemoteAET" and
   * "IndexInSeries".
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @param metadata The metadata of interest.
   * @return 1 if the metadata is present, 0 if it is absent, -1 in case of error.
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE int  OrthancPluginHasInstanceMetadata(
    OrthancPluginContext*              context,
    const OrthancPluginDicomInstance*  instance,
    const char*                        metadata)
  {
    int64_t result;

    _OrthancPluginAccessDicomInstance params;
    memset(&params, 0, sizeof(params));
    params.resultInt64 = &result;
    params.instance = instance;
    params.key = metadata;

    if (context->InvokeService(context, _OrthancPluginService_HasInstanceMetadata, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return -1;
    }
    else
    {
      return (result != 0);
    }
  }


  /**
   * @brief Get the value of some metadata associated with a given DICOM instance.
   *
   * This functions returns the value of some metadata that is associated with the DICOM instance of interest.
   * Before calling this function, the existence of the metadata must have been checked with
   * ::OrthancPluginHasInstanceMetadata().
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @param metadata The metadata of interest.
   * @return The metadata value if success, NULL if error. Please note that the 
   *         returned string belongs to the instance object and must NOT be 
   *         deallocated. Please make a copy of the string if you wish to access 
   *         it later.
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE const char* OrthancPluginGetInstanceMetadata(
    OrthancPluginContext*              context,
    const OrthancPluginDicomInstance*  instance,
    const char*                        metadata)
  {
    const char* result;

    _OrthancPluginAccessDicomInstance params;
    memset(&params, 0, sizeof(params));
    params.resultString = &result;
    params.instance = instance;
    params.key = metadata;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceMetadata, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }



  typedef struct
  {
    OrthancPluginStorageCreate  create;
    OrthancPluginStorageRead    read;
    OrthancPluginStorageRemove  remove;
    OrthancPluginFree           free;
  } _OrthancPluginRegisterStorageArea;

  /**
   * @brief Register a custom storage area.
   *
   * This function registers a custom storage area, to replace the
   * built-in way Orthanc stores its files on the filesystem. This
   * function must be called during the initialization of the plugin,
   * i.e. inside the OrthancPluginInitialize() public function.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param create The callback function to store a file on the custom storage area.
   * @param read The callback function to read a file from the custom storage area.
   * @param remove The callback function to remove a file from the custom storage area.
   * @ingroup Callbacks
   * @deprecated Please use OrthancPluginRegisterStorageArea2()
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginRegisterStorageArea(
    OrthancPluginContext*       context,
    OrthancPluginStorageCreate  create,
    OrthancPluginStorageRead    read,
    OrthancPluginStorageRemove  remove)
  {
    _OrthancPluginRegisterStorageArea params;
    params.create = create;
    params.read = read;
    params.remove = remove;

#ifdef  __cplusplus
    params.free = ::free;
#else
    params.free = free;
#endif

    context->InvokeService(context, _OrthancPluginService_RegisterStorageArea, &params);
  }



  /**
   * @brief Return the path to the Orthanc executable.
   *
   * This function returns the path to the Orthanc executable.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return NULL in the case of an error, or a newly allocated string
   * containing the path. This string must be freed by
   * OrthancPluginFreeString().
   **/
  ORTHANC_PLUGIN_INLINE char *OrthancPluginGetOrthancPath(OrthancPluginContext* context)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = NULL;

    if (context->InvokeService(context, _OrthancPluginService_GetOrthancPath, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Return the directory containing the Orthanc.
   *
   * This function returns the path to the directory containing the Orthanc executable.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return NULL in the case of an error, or a newly allocated string
   * containing the path. This string must be freed by
   * OrthancPluginFreeString().
   **/
  ORTHANC_PLUGIN_INLINE char *OrthancPluginGetOrthancDirectory(OrthancPluginContext* context)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = NULL;

    if (context->InvokeService(context, _OrthancPluginService_GetOrthancDirectory, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Return the path to the configuration file(s).
   *
   * This function returns the path to the configuration file(s) that
   * was specified when starting Orthanc. Since version 0.9.1, this
   * path can refer to a folder that stores a set of configuration
   * files. This function is deprecated in favor of
   * OrthancPluginGetConfiguration().
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return NULL in the case of an error, or a newly allocated string
   * containing the path. This string must be freed by
   * OrthancPluginFreeString().
   * @see OrthancPluginGetConfiguration()
   **/
  ORTHANC_PLUGIN_INLINE char *OrthancPluginGetConfigurationPath(OrthancPluginContext* context)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = NULL;

    if (context->InvokeService(context, _OrthancPluginService_GetConfigurationPath, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }



  typedef struct
  {
    OrthancPluginOnChangeCallback callback;
  } _OrthancPluginOnChangeCallback;

  /**
   * @brief Register a callback to monitor changes.
   *
   * This function registers a callback function that is called
   * whenever a change happens to some DICOM resource.
   *
   * @warning Your callback function will be called synchronously with
   * the core of Orthanc. This implies that deadlocks might emerge if
   * you call other core primitives of Orthanc in your callback (such
   * deadlocks are particularly visible in the presence of other plugins
   * or Lua scripts). It is thus strongly advised to avoid any call to
   * the REST API of Orthanc in the callback. If you have to call
   * other primitives of Orthanc, you should make these calls in a
   * separate thread, passing the pending events to be processed
   * through a message queue.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback function.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginRegisterOnChangeCallback(
    OrthancPluginContext*          context,
    OrthancPluginOnChangeCallback  callback)
  {
    _OrthancPluginOnChangeCallback params;
    params.callback = callback;

    context->InvokeService(context, _OrthancPluginService_RegisterOnChangeCallback, &params);
  }



  typedef struct
  {
    const char* plugin;
    _OrthancPluginProperty property;
    const char* value;
  } _OrthancPluginSetPluginProperty;


  /**
   * @brief Set the URI where the plugin provides its Web interface.
   *
   * For plugins that come with a Web interface, this function
   * declares the entry path where to find this interface. This
   * information is notably used in the "Plugins" page of Orthanc
   * Explorer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param uri The root URI for this plugin.
   **/ 
  ORTHANC_PLUGIN_INLINE void OrthancPluginSetRootUri(
    OrthancPluginContext*  context,
    const char*            uri)
  {
    _OrthancPluginSetPluginProperty params;
    params.plugin = OrthancPluginGetName();
    params.property = _OrthancPluginProperty_RootUri;
    params.value = uri;

    context->InvokeService(context, _OrthancPluginService_SetPluginProperty, &params);
  }


  /**
   * @brief Set a description for this plugin.
   *
   * Set a description for this plugin. It is displayed in the
   * "Plugins" page of Orthanc Explorer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param description The description.
   **/ 
  ORTHANC_PLUGIN_INLINE void OrthancPluginSetDescription(
    OrthancPluginContext*  context,
    const char*            description)
  {
    _OrthancPluginSetPluginProperty params;
    params.plugin = OrthancPluginGetName();
    params.property = _OrthancPluginProperty_Description;
    params.value = description;

    context->InvokeService(context, _OrthancPluginService_SetPluginProperty, &params);
  }


  /**
   * @brief Extend the JavaScript code of Orthanc Explorer.
   *
   * Add JavaScript code to customize the default behavior of Orthanc
   * Explorer. This can for instance be used to add new buttons.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param javascript The custom JavaScript code.
   **/ 
  ORTHANC_PLUGIN_INLINE void OrthancPluginExtendOrthancExplorer(
    OrthancPluginContext*  context,
    const char*            javascript)
  {
    _OrthancPluginSetPluginProperty params;
    params.plugin = OrthancPluginGetName();
    params.property = _OrthancPluginProperty_OrthancExplorer;
    params.value = javascript;

    context->InvokeService(context, _OrthancPluginService_SetPluginProperty, &params);
  }


  typedef struct
  {
    char**       result;
    int32_t      property;
    const char*  value;
  } _OrthancPluginGlobalProperty;


  /**
   * @brief Get the value of a global property.
   *
   * Get the value of a global property that is stored in the Orthanc database. Global
   * properties whose index is below 1024 are reserved by Orthanc.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param property The global property of interest.
   * @param defaultValue The value to return, if the global property is unset.
   * @return The value of the global property, or NULL in the case of an error. This
   * string must be freed by OrthancPluginFreeString().
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGetGlobalProperty(
    OrthancPluginContext*  context,
    int32_t                property,
    const char*            defaultValue)
  {
    char* result;

    _OrthancPluginGlobalProperty params;
    params.result = &result;
    params.property = property;
    params.value = defaultValue;

    if (context->InvokeService(context, _OrthancPluginService_GetGlobalProperty, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Set the value of a global property.
   *
   * Set the value of a global property into the Orthanc
   * database. Setting a global property can be used by plugins to
   * save their internal parameters. Plugins are only allowed to set
   * properties whose index are above or equal to 1024 (properties
   * below 1024 are read-only and reserved by Orthanc).
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param property The global property of interest.
   * @param value The value to be set in the global property.
   * @return 0 if success, or the error code if failure.
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginSetGlobalProperty(
    OrthancPluginContext*  context,
    int32_t                property,
    const char*            value)
  {
    _OrthancPluginGlobalProperty params;
    params.result = NULL;
    params.property = property;
    params.value = value;

    return context->InvokeService(context, _OrthancPluginService_SetGlobalProperty, &params);
  }



  typedef struct
  {
    int32_t   *resultInt32;
    uint32_t  *resultUint32;
    int64_t   *resultInt64;
    uint64_t  *resultUint64;
  } _OrthancPluginReturnSingleValue;

  /**
   * @brief Get the number of command-line arguments.
   *
   * Retrieve the number of command-line arguments that were used to launch Orthanc.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return The number of arguments.
   **/
  ORTHANC_PLUGIN_INLINE uint32_t OrthancPluginGetCommandLineArgumentsCount(
    OrthancPluginContext*  context)
  {
    uint32_t count = 0;

    _OrthancPluginReturnSingleValue params;
    memset(&params, 0, sizeof(params));
    params.resultUint32 = &count;

    if (context->InvokeService(context, _OrthancPluginService_GetCommandLineArgumentsCount, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return 0;
    }
    else
    {
      return count;
    }
  }



  /**
   * @brief Get the value of a command-line argument.
   *
   * Get the value of one of the command-line arguments that were used
   * to launch Orthanc. The number of available arguments can be
   * retrieved by OrthancPluginGetCommandLineArgumentsCount().
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param argument The index of the argument.
   * @return The value of the argument, or NULL in the case of an error. This
   * string must be freed by OrthancPluginFreeString().
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGetCommandLineArgument(
    OrthancPluginContext*  context,
    uint32_t               argument)
  {
    char* result;

    _OrthancPluginGlobalProperty params;
    params.result = &result;
    params.property = (int32_t) argument;
    params.value = NULL;

    if (context->InvokeService(context, _OrthancPluginService_GetCommandLineArgument, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Get the expected version of the database schema.
   *
   * Retrieve the expected version of the database schema.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return The version.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE uint32_t OrthancPluginGetExpectedDatabaseVersion(
    OrthancPluginContext*  context)
  {
    uint32_t count = 0;

    _OrthancPluginReturnSingleValue params;
    memset(&params, 0, sizeof(params));
    params.resultUint32 = &count;

    if (context->InvokeService(context, _OrthancPluginService_GetExpectedDatabaseVersion, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return 0;
    }
    else
    {
      return count;
    }
  }



  /**
   * @brief Return the content of the configuration file(s).
   *
   * This function returns the content of the configuration that is
   * used by Orthanc, formatted as a JSON string.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return NULL in the case of an error, or a newly allocated string
   * containing the configuration. This string must be freed by
   * OrthancPluginFreeString().
   **/
  ORTHANC_PLUGIN_INLINE char *OrthancPluginGetConfiguration(OrthancPluginContext* context)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = NULL;

    if (context->InvokeService(context, _OrthancPluginService_GetConfiguration, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }



  typedef struct
  {
    OrthancPluginRestOutput* output;
    const char*              subType;
    const char*              contentType;
  } _OrthancPluginStartMultipartAnswer;

  /**
   * @brief Start an HTTP multipart answer.
   *
   * Initiates a HTTP multipart answer, as the result of a REST request.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param subType The sub-type of the multipart answer ("mixed" or "related").
   * @param contentType The MIME type of the items in the multipart answer.
   * @return 0 if success, or the error code if failure.
   * @see OrthancPluginSendMultipartItem(), OrthancPluginSendMultipartItem2()
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginStartMultipartAnswer(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    const char*              subType,
    const char*              contentType)
  {
    _OrthancPluginStartMultipartAnswer params;
    params.output = output;
    params.subType = subType;
    params.contentType = contentType;
    return context->InvokeService(context, _OrthancPluginService_StartMultipartAnswer, &params);
  }


  /**
   * @brief Send an item as a part of some HTTP multipart answer.
   *
   * This function sends an item as a part of some HTTP multipart
   * answer that was initiated by OrthancPluginStartMultipartAnswer().
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param answer Pointer to the memory buffer containing the item.
   * @param answerSize Number of bytes of the item.
   * @return 0 if success, or the error code if failure (this notably happens
   * if the connection is closed by the client).
   * @see OrthancPluginSendMultipartItem2()
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginSendMultipartItem(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    const void*              answer,
    uint32_t                 answerSize)
  {
    _OrthancPluginAnswerBuffer params;
    params.output = output;
    params.answer = answer;
    params.answerSize = answerSize;
    params.mimeType = NULL;
    return context->InvokeService(context, _OrthancPluginService_SendMultipartItem, &params);
  }



  typedef struct
  {
    OrthancPluginMemoryBuffer*    target;
    const void*                   source;
    uint32_t                      size;
    OrthancPluginCompressionType  compression;
    uint8_t                       uncompress;
  } _OrthancPluginBufferCompression;


  /**
   * @brief Compress or decompress a buffer.
   *
   * This function compresses or decompresses a buffer, using the
   * version of the zlib library that is used by the Orthanc core.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param source The source buffer.
   * @param size The size in bytes of the source buffer.
   * @param compression The compression algorithm.
   * @param uncompress If set to "0", the buffer must be compressed. 
   * If set to "1", the buffer must be uncompressed.
   * @return 0 if success, or the error code if failure.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginBufferCompression(
    OrthancPluginContext*         context,
    OrthancPluginMemoryBuffer*    target,
    const void*                   source,
    uint32_t                      size,
    OrthancPluginCompressionType  compression,
    uint8_t                       uncompress)
  {
    _OrthancPluginBufferCompression params;
    params.target = target;
    params.source = source;
    params.size = size;
    params.compression = compression;
    params.uncompress = uncompress;

    return context->InvokeService(context, _OrthancPluginService_BufferCompression, &params);
  }



  typedef struct
  {
    OrthancPluginMemoryBuffer*  target;
    const char*                 path;
  } _OrthancPluginReadFile;

  /**
   * @brief Read a file.
   * 
   * Read the content of a file on the filesystem, and returns it into
   * a newly allocated memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param path The path of the file to be read.
   * @return 0 if success, or the error code if failure.
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginReadFile(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 path)
  {
    _OrthancPluginReadFile params;
    params.target = target;
    params.path = path;
    return context->InvokeService(context, _OrthancPluginService_ReadFile, &params);
  }



  typedef struct
  {
    const char*  path;
    const void*  data;
    uint32_t     size;
  } _OrthancPluginWriteFile;

  /**
   * @brief Write a file.
   * 
   * Write the content of a memory buffer to the filesystem.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param path The path of the file to be written.
   * @param data The content of the memory buffer.
   * @param size The size of the memory buffer.
   * @return 0 if success, or the error code if failure.
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginWriteFile(
    OrthancPluginContext*  context,
    const char*            path,
    const void*            data,
    uint32_t               size)
  {
    _OrthancPluginWriteFile params;
    params.path = path;
    params.data = data;
    params.size = size;
    return context->InvokeService(context, _OrthancPluginService_WriteFile, &params);
  }



  typedef struct
  {
    const char**            target;
    OrthancPluginErrorCode  error;
  } _OrthancPluginGetErrorDescription;

  /**
   * @brief Get the description of a given error code.
   *
   * This function returns the description of a given error code.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param error The error code of interest.
   * @return The error description. This is a statically-allocated
   * string, do not free it.
   **/
  ORTHANC_PLUGIN_INLINE const char* OrthancPluginGetErrorDescription(
    OrthancPluginContext*    context,
    OrthancPluginErrorCode   error)
  {
    const char* result = NULL;

    _OrthancPluginGetErrorDescription params;
    params.target = &result;
    params.error = error;

    if (context->InvokeService(context, _OrthancPluginService_GetErrorDescription, &params) != OrthancPluginErrorCode_Success ||
        result == NULL)
    {
      return "Unknown error code";
    }
    else
    {
      return result;
    }
  }



  typedef struct
  {
    OrthancPluginRestOutput* output;
    uint16_t                 status;
    const void*              body;
    uint32_t                 bodySize;
  } _OrthancPluginSendHttpStatus;

  /**
   * @brief Send a HTTP status, with a custom body.
   *
   * This function answers to a HTTP request by sending a HTTP status
   * code (such as "400 - Bad Request"), together with a body
   * describing the error. The body will only be returned if the
   * configuration option "HttpDescribeErrors" of Orthanc is set to "true".
   * 
   * Note that:
   * - Successful requests (status 200) must use ::OrthancPluginAnswerBuffer().
   * - Redirections (status 301) must use ::OrthancPluginRedirect().
   * - Unauthorized access (status 401) must use ::OrthancPluginSendUnauthorized().
   * - Methods not allowed (status 405) must use ::OrthancPluginSendMethodNotAllowed().
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param status The HTTP status code to be sent.
   * @param body The body of the answer.
   * @param bodySize The size of the body.
   * @see OrthancPluginSendHttpStatusCode()
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginSendHttpStatus(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    uint16_t                 status,
    const void*              body,
    uint32_t                 bodySize)
  {
    _OrthancPluginSendHttpStatus params;
    params.output = output;
    params.status = status;
    params.body = body;
    params.bodySize = bodySize;
    context->InvokeService(context, _OrthancPluginService_SendHttpStatus, &params);
  }



  typedef struct
  {
    const OrthancPluginImage*  image;
    uint32_t*                  resultUint32;
    OrthancPluginPixelFormat*  resultPixelFormat;
    void**                     resultBuffer;
  } _OrthancPluginGetImageInfo;


  /**
   * @brief Return the pixel format of an image.
   *
   * This function returns the type of memory layout for the pixels of the given image.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param image The image of interest.
   * @return The pixel format.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginPixelFormat  OrthancPluginGetImagePixelFormat(
    OrthancPluginContext*      context,
    const OrthancPluginImage*  image)
  {
    OrthancPluginPixelFormat target;
    
    _OrthancPluginGetImageInfo params;
    memset(&params, 0, sizeof(params));
    params.image = image;
    params.resultPixelFormat = &target;

    if (context->InvokeService(context, _OrthancPluginService_GetImagePixelFormat, &params) != OrthancPluginErrorCode_Success)
    {
      return OrthancPluginPixelFormat_Unknown;
    }
    else
    {
      return (OrthancPluginPixelFormat) target;
    }
  }



  /**
   * @brief Return the width of an image.
   *
   * This function returns the width of the given image.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param image The image of interest.
   * @return The width.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE uint32_t  OrthancPluginGetImageWidth(
    OrthancPluginContext*      context,
    const OrthancPluginImage*  image)
  {
    uint32_t width;
    
    _OrthancPluginGetImageInfo params;
    memset(&params, 0, sizeof(params));
    params.image = image;
    params.resultUint32 = &width;

    if (context->InvokeService(context, _OrthancPluginService_GetImageWidth, &params) != OrthancPluginErrorCode_Success)
    {
      return 0;
    }
    else
    {
      return width;
    }
  }



  /**
   * @brief Return the height of an image.
   *
   * This function returns the height of the given image.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param image The image of interest.
   * @return The height.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE uint32_t  OrthancPluginGetImageHeight(
    OrthancPluginContext*      context,
    const OrthancPluginImage*  image)
  {
    uint32_t height;
    
    _OrthancPluginGetImageInfo params;
    memset(&params, 0, sizeof(params));
    params.image = image;
    params.resultUint32 = &height;

    if (context->InvokeService(context, _OrthancPluginService_GetImageHeight, &params) != OrthancPluginErrorCode_Success)
    {
      return 0;
    }
    else
    {
      return height;
    }
  }



  /**
   * @brief Return the pitch of an image.
   *
   * This function returns the pitch of the given image. The pitch is
   * defined as the number of bytes between 2 successive lines of the
   * image in the memory buffer.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param image The image of interest.
   * @return The pitch.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE uint32_t  OrthancPluginGetImagePitch(
    OrthancPluginContext*      context,
    const OrthancPluginImage*  image)
  {
    uint32_t pitch;
    
    _OrthancPluginGetImageInfo params;
    memset(&params, 0, sizeof(params));
    params.image = image;
    params.resultUint32 = &pitch;

    if (context->InvokeService(context, _OrthancPluginService_GetImagePitch, &params) != OrthancPluginErrorCode_Success)
    {
      return 0;
    }
    else
    {
      return pitch;
    }
  }



  /**
   * @brief Return a pointer to the content of an image.
   *
   * This function returns a pointer to the memory buffer that
   * contains the pixels of the image.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param image The image of interest.
   * @return The pointer.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE void*  OrthancPluginGetImageBuffer(
    OrthancPluginContext*      context,
    const OrthancPluginImage*  image)
  {
    void* target = NULL;

    _OrthancPluginGetImageInfo params;
    memset(&params, 0, sizeof(params));
    params.resultBuffer = &target;
    params.image = image;

    if (context->InvokeService(context, _OrthancPluginService_GetImageBuffer, &params) != OrthancPluginErrorCode_Success)
    {
      return NULL;
    }
    else
    {
      return target;
    }
  }


  typedef struct
  {
    OrthancPluginImage**       target;
    const void*                data;
    uint32_t                   size;
    OrthancPluginImageFormat   format;
  } _OrthancPluginUncompressImage;


  /**
   * @brief Decode a compressed image.
   *
   * This function decodes a compressed image from a memory buffer.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param data Pointer to a memory buffer containing the compressed image.
   * @param size Size of the memory buffer containing the compressed image.
   * @param format The file format of the compressed image.
   * @return The uncompressed image. It must be freed with OrthancPluginFreeImage().
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginImage *OrthancPluginUncompressImage(
    OrthancPluginContext*      context,
    const void*                data,
    uint32_t                   size,
    OrthancPluginImageFormat   format)
  {
    OrthancPluginImage* target = NULL;

    _OrthancPluginUncompressImage params;
    memset(&params, 0, sizeof(params));
    params.target = &target;
    params.data = data;
    params.size = size;
    params.format = format;

    if (context->InvokeService(context, _OrthancPluginService_UncompressImage, &params) != OrthancPluginErrorCode_Success)
    {
      return NULL;
    }
    else
    {
      return target;
    }
  }




  typedef struct
  {
    OrthancPluginImage*   image;
  } _OrthancPluginFreeImage;

  /**
   * @brief Free an image.
   *
   * This function frees an image that was decoded with OrthancPluginUncompressImage().
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param image The image.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE void  OrthancPluginFreeImage(
    OrthancPluginContext* context, 
    OrthancPluginImage*   image)
  {
    _OrthancPluginFreeImage params;
    params.image = image;

    context->InvokeService(context, _OrthancPluginService_FreeImage, &params);
  }




  typedef struct
  {
    OrthancPluginMemoryBuffer* target;
    OrthancPluginImageFormat   imageFormat;
    OrthancPluginPixelFormat   pixelFormat;
    uint32_t                   width;
    uint32_t                   height;
    uint32_t                   pitch;
    const void*                buffer;
    uint8_t                    quality;
  } _OrthancPluginCompressImage;


  /**
   * @brief Encode a PNG image.
   *
   * This function compresses the given memory buffer containing an
   * image using the PNG specification, and stores the result of the
   * compression into a newly allocated memory buffer.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param format The memory layout of the uncompressed image.
   * @param width The width of the image.
   * @param height The height of the image.
   * @param pitch The pitch of the image (i.e. the number of bytes
   * between 2 successive lines of the image in the memory buffer).
   * @param buffer The memory buffer containing the uncompressed image.
   * @return 0 if success, or the error code if failure.
   * @see OrthancPluginCompressAndAnswerPngImage()
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginCompressPngImage(
    OrthancPluginContext*         context,
    OrthancPluginMemoryBuffer*    target,
    OrthancPluginPixelFormat      format,
    uint32_t                      width,
    uint32_t                      height,
    uint32_t                      pitch,
    const void*                   buffer)
  {
    _OrthancPluginCompressImage params;
    memset(&params, 0, sizeof(params));
    params.target = target;
    params.imageFormat = OrthancPluginImageFormat_Png;
    params.pixelFormat = format;
    params.width = width;
    params.height = height;
    params.pitch = pitch;
    params.buffer = buffer;
    params.quality = 0;  /* Unused for PNG */

    return context->InvokeService(context, _OrthancPluginService_CompressImage, &params);
  }


  /**
   * @brief Encode a JPEG image.
   *
   * This function compresses the given memory buffer containing an
   * image using the JPEG specification, and stores the result of the
   * compression into a newly allocated memory buffer.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param format The memory layout of the uncompressed image.
   * @param width The width of the image.
   * @param height The height of the image.
   * @param pitch The pitch of the image (i.e. the number of bytes
   * between 2 successive lines of the image in the memory buffer).
   * @param buffer The memory buffer containing the uncompressed image.
   * @param quality The quality of the JPEG encoding, between 1 (worst
   * quality, best compression) and 100 (best quality, worst
   * compression).
   * @return 0 if success, or the error code if failure.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginCompressJpegImage(
    OrthancPluginContext*         context,
    OrthancPluginMemoryBuffer*    target,
    OrthancPluginPixelFormat      format,
    uint32_t                      width,
    uint32_t                      height,
    uint32_t                      pitch,
    const void*                   buffer,
    uint8_t                       quality)
  {
    _OrthancPluginCompressImage params;
    memset(&params, 0, sizeof(params));
    params.target = target;
    params.imageFormat = OrthancPluginImageFormat_Jpeg;
    params.pixelFormat = format;
    params.width = width;
    params.height = height;
    params.pitch = pitch;
    params.buffer = buffer;
    params.quality = quality;

    return context->InvokeService(context, _OrthancPluginService_CompressImage, &params);
  }



  /**
   * @brief Answer to a REST request with a JPEG image.
   *
   * This function answers to a REST request with a JPEG image. The
   * parameters of this function describe a memory buffer that
   * contains an uncompressed image. The image will be automatically compressed
   * as a JPEG image by the core system of Orthanc.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param format The memory layout of the uncompressed image.
   * @param width The width of the image.
   * @param height The height of the image.
   * @param pitch The pitch of the image (i.e. the number of bytes
   * between 2 successive lines of the image in the memory buffer).
   * @param buffer The memory buffer containing the uncompressed image.
   * @param quality The quality of the JPEG encoding, between 1 (worst
   * quality, best compression) and 100 (best quality, worst
   * compression).
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginCompressAndAnswerJpegImage(
    OrthancPluginContext*     context,
    OrthancPluginRestOutput*  output,
    OrthancPluginPixelFormat  format,
    uint32_t                  width,
    uint32_t                  height,
    uint32_t                  pitch,
    const void*               buffer,
    uint8_t                   quality)
  {
    _OrthancPluginCompressAndAnswerImage params;
    params.output = output;
    params.imageFormat = OrthancPluginImageFormat_Jpeg;
    params.pixelFormat = format;
    params.width = width;
    params.height = height;
    params.pitch = pitch;
    params.buffer = buffer;
    params.quality = quality;
    context->InvokeService(context, _OrthancPluginService_CompressAndAnswerImage, &params);
  }




  typedef struct
  {
    OrthancPluginMemoryBuffer*  target;
    OrthancPluginHttpMethod     method;
    const char*                 url;
    const char*                 username;
    const char*                 password;
    const void*                 body;
    uint32_t                    bodySize;
  } _OrthancPluginCallHttpClient;


  /**
   * @brief Issue a HTTP GET call.
   * 
   * Make a HTTP GET call to the given URL. The result to the query is
   * stored into a newly allocated memory buffer. Favor
   * OrthancPluginRestApiGet() if calling the built-in REST API of the
   * Orthanc instance that hosts this plugin.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param url The URL of interest.
   * @param username The username (can be <tt>NULL</tt> if no password protection).
   * @param password The password (can be <tt>NULL</tt> if no password protection).
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginHttpGet(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 url,
    const char*                 username,
    const char*                 password)
  {
    _OrthancPluginCallHttpClient params;
    memset(&params, 0, sizeof(params));

    params.target = target;
    params.method = OrthancPluginHttpMethod_Get;
    params.url = url;
    params.username = username;
    params.password = password;

    return context->InvokeService(context, _OrthancPluginService_CallHttpClient, &params);
  }


  /**
   * @brief Issue a HTTP POST call.
   * 
   * Make a HTTP POST call to the given URL. The result to the query
   * is stored into a newly allocated memory buffer. Favor
   * OrthancPluginRestApiPost() if calling the built-in REST API of
   * the Orthanc instance that hosts this plugin.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param url The URL of interest.
   * @param body The content of the body of the request.
   * @param bodySize The size of the body of the request.
   * @param username The username (can be <tt>NULL</tt> if no password protection).
   * @param password The password (can be <tt>NULL</tt> if no password protection).
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginHttpPost(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 url,
    const void*                 body,
    uint32_t                    bodySize,
    const char*                 username,
    const char*                 password)
  {
    _OrthancPluginCallHttpClient params;
    memset(&params, 0, sizeof(params));

    params.target = target;
    params.method = OrthancPluginHttpMethod_Post;
    params.url = url;
    params.body = body;
    params.bodySize = bodySize;
    params.username = username;
    params.password = password;

    return context->InvokeService(context, _OrthancPluginService_CallHttpClient, &params);
  }


  /**
   * @brief Issue a HTTP PUT call.
   * 
   * Make a HTTP PUT call to the given URL. The result to the query is
   * stored into a newly allocated memory buffer. Favor
   * OrthancPluginRestApiPut() if calling the built-in REST API of the
   * Orthanc instance that hosts this plugin.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param url The URL of interest.
   * @param body The content of the body of the request.
   * @param bodySize The size of the body of the request.
   * @param username The username (can be <tt>NULL</tt> if no password protection).
   * @param password The password (can be <tt>NULL</tt> if no password protection).
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginHttpPut(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 url,
    const void*                 body,
    uint32_t                    bodySize,
    const char*                 username,
    const char*                 password)
  {
    _OrthancPluginCallHttpClient params;
    memset(&params, 0, sizeof(params));

    params.target = target;
    params.method = OrthancPluginHttpMethod_Put;
    params.url = url;
    params.body = body;
    params.bodySize = bodySize;
    params.username = username;
    params.password = password;

    return context->InvokeService(context, _OrthancPluginService_CallHttpClient, &params);
  }


  /**
   * @brief Issue a HTTP DELETE call.
   * 
   * Make a HTTP DELETE call to the given URL. Favor
   * OrthancPluginRestApiDelete() if calling the built-in REST API of
   * the Orthanc instance that hosts this plugin.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param url The URL of interest.
   * @param username The username (can be <tt>NULL</tt> if no password protection).
   * @param password The password (can be <tt>NULL</tt> if no password protection).
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginHttpDelete(
    OrthancPluginContext*       context,
    const char*                 url,
    const char*                 username,
    const char*                 password)
  {
    _OrthancPluginCallHttpClient params;
    memset(&params, 0, sizeof(params));

    params.method = OrthancPluginHttpMethod_Delete;
    params.url = url;
    params.username = username;
    params.password = password;

    return context->InvokeService(context, _OrthancPluginService_CallHttpClient, &params);
  }



  typedef struct
  {
    OrthancPluginImage**       target;
    const OrthancPluginImage*  source;
    OrthancPluginPixelFormat   targetFormat;
  } _OrthancPluginConvertPixelFormat;


  /**
   * @brief Change the pixel format of an image.
   *
   * This function creates a new image, changing the memory layout of the pixels.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param source The source image.
   * @param targetFormat The target pixel format.
   * @return The resulting image. It must be freed with OrthancPluginFreeImage().
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginImage *OrthancPluginConvertPixelFormat(
    OrthancPluginContext*      context,
    const OrthancPluginImage*  source,
    OrthancPluginPixelFormat   targetFormat)
  {
    OrthancPluginImage* target = NULL;

    _OrthancPluginConvertPixelFormat params;
    params.target = &target;
    params.source = source;
    params.targetFormat = targetFormat;

    if (context->InvokeService(context, _OrthancPluginService_ConvertPixelFormat, &params) != OrthancPluginErrorCode_Success)
    {
      return NULL;
    }
    else
    {
      return target;
    }
  }



  /**
   * @brief Return the number of available fonts.
   *
   * This function returns the number of fonts that are built in the
   * Orthanc core. These fonts can be used to draw texts on images
   * through OrthancPluginDrawText().
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return The number of fonts.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE uint32_t OrthancPluginGetFontsCount(
    OrthancPluginContext*  context)
  {
    uint32_t count = 0;

    _OrthancPluginReturnSingleValue params;
    memset(&params, 0, sizeof(params));
    params.resultUint32 = &count;

    if (context->InvokeService(context, _OrthancPluginService_GetFontsCount, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return 0;
    }
    else
    {
      return count;
    }
  }




  typedef struct
  {
    uint32_t      fontIndex; /* in */
    const char**  name; /* out */
    uint32_t*     size; /* out */
  } _OrthancPluginGetFontInfo;

  /**
   * @brief Return the name of a font.
   *
   * This function returns the name of a font that is built in the Orthanc core.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param fontIndex The index of the font. This value must be less than OrthancPluginGetFontsCount().
   * @return The font name. This is a statically-allocated string, do not free it.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE const char* OrthancPluginGetFontName(
    OrthancPluginContext*  context,
    uint32_t               fontIndex)
  {
    const char* result = NULL;

    _OrthancPluginGetFontInfo params;
    memset(&params, 0, sizeof(params));
    params.name = &result;
    params.fontIndex = fontIndex;

    if (context->InvokeService(context, _OrthancPluginService_GetFontInfo, &params) != OrthancPluginErrorCode_Success)
    {
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Return the size of a font.
   *
   * This function returns the size of a font that is built in the Orthanc core.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param fontIndex The index of the font. This value must be less than OrthancPluginGetFontsCount().
   * @return The font size.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE uint32_t OrthancPluginGetFontSize(
    OrthancPluginContext*  context,
    uint32_t               fontIndex)
  {
    uint32_t result;

    _OrthancPluginGetFontInfo params;
    memset(&params, 0, sizeof(params));
    params.size = &result;
    params.fontIndex = fontIndex;

    if (context->InvokeService(context, _OrthancPluginService_GetFontInfo, &params) != OrthancPluginErrorCode_Success)
    {
      return 0;
    }
    else
    {
      return result;
    }
  }



  typedef struct
  {
    OrthancPluginImage*   image;
    uint32_t              fontIndex;
    const char*           utf8Text;
    int32_t               x;
    int32_t               y;
    uint8_t               r;
    uint8_t               g;
    uint8_t               b;
  } _OrthancPluginDrawText;


  /**
   * @brief Draw text on an image.
   *
   * This function draws some text on some image.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param image The image upon which to draw the text.
   * @param fontIndex The index of the font. This value must be less than OrthancPluginGetFontsCount().
   * @param utf8Text The text to be drawn, encoded as an UTF-8 zero-terminated string.
   * @param x The X position of the text over the image.
   * @param y The Y position of the text over the image.
   * @param r The value of the red color channel of the text.
   * @param g The value of the green color channel of the text.
   * @param b The value of the blue color channel of the text.
   * @return 0 if success, other value if error.
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginDrawText(
    OrthancPluginContext*  context,
    OrthancPluginImage*    image,
    uint32_t               fontIndex,
    const char*            utf8Text,
    int32_t                x,
    int32_t                y,
    uint8_t                r,
    uint8_t                g,
    uint8_t                b)
  {
    _OrthancPluginDrawText params;
    memset(&params, 0, sizeof(params));
    params.image = image;
    params.fontIndex = fontIndex;
    params.utf8Text = utf8Text;
    params.x = x;
    params.y = y;
    params.r = r;
    params.g = g;
    params.b = b;

    return context->InvokeService(context, _OrthancPluginService_DrawText, &params);
  }



  typedef struct
  {
    OrthancPluginStorageArea*   storageArea;
    const char*                 uuid;
    const void*                 content;
    uint64_t                    size;
    OrthancPluginContentType    type;
  } _OrthancPluginStorageAreaCreate;


  /**
   * @brief Create a file inside the storage area.
   *
   * This function creates a new file inside the storage area that is
   * currently used by Orthanc.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param storageArea The storage area.
   * @param uuid The identifier of the file to be created.
   * @param content The content to store in the newly created file.
   * @param size The size of the content.
   * @param type The type of the file content.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   * @deprecated This function should not be used anymore. Use "OrthancPluginRestApiPut()" on
   * "/{patients|studies|series|instances}/{id}/attachments/{name}" instead.
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginStorageAreaCreate(
    OrthancPluginContext*       context,
    OrthancPluginStorageArea*   storageArea,
    const char*                 uuid,
    const void*                 content,
    uint64_t                    size,
    OrthancPluginContentType    type)
  {
    _OrthancPluginStorageAreaCreate params;
    params.storageArea = storageArea;
    params.uuid = uuid;
    params.content = content;
    params.size = size;
    params.type = type;

    return context->InvokeService(context, _OrthancPluginService_StorageAreaCreate, &params);
  }


  typedef struct
  {
    OrthancPluginMemoryBuffer*  target;
    OrthancPluginStorageArea*   storageArea;
    const char*                 uuid;
    OrthancPluginContentType    type;
  } _OrthancPluginStorageAreaRead;


  /**
   * @brief Read a file from the storage area.
   *
   * This function reads the content of a given file from the storage
   * area that is currently used by Orthanc.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param storageArea The storage area.
   * @param uuid The identifier of the file to be read.
   * @param type The type of the file content.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   * @deprecated This function should not be used anymore. Use "OrthancPluginRestApiGet()" on
   * "/{patients|studies|series|instances}/{id}/attachments/{name}" instead.
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginStorageAreaRead(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    OrthancPluginStorageArea*   storageArea,
    const char*                 uuid,
    OrthancPluginContentType    type)
  {
    _OrthancPluginStorageAreaRead params;
    params.target = target;
    params.storageArea = storageArea;
    params.uuid = uuid;
    params.type = type;

    return context->InvokeService(context, _OrthancPluginService_StorageAreaRead, &params);
  }


  typedef struct
  {
    OrthancPluginStorageArea*   storageArea;
    const char*                 uuid;
    OrthancPluginContentType    type;
  } _OrthancPluginStorageAreaRemove;

  /**
   * @brief Remove a file from the storage area.
   *
   * This function removes a given file from the storage area that is
   * currently used by Orthanc.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param storageArea The storage area.
   * @param uuid The identifier of the file to be removed.
   * @param type The type of the file content.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   * @deprecated This function should not be used anymore. Use "OrthancPluginRestApiDelete()" on
   * "/{patients|studies|series|instances}/{id}/attachments/{name}" instead.
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginStorageAreaRemove(
    OrthancPluginContext*       context,
    OrthancPluginStorageArea*   storageArea,
    const char*                 uuid,
    OrthancPluginContentType    type)
  {
    _OrthancPluginStorageAreaRemove params;
    params.storageArea = storageArea;
    params.uuid = uuid;
    params.type = type;

    return context->InvokeService(context, _OrthancPluginService_StorageAreaRemove, &params);
  }



  typedef struct
  {
    OrthancPluginErrorCode*  target;
    int32_t                  code;
    uint16_t                 httpStatus;
    const char*              message;
  } _OrthancPluginRegisterErrorCode;
  
  /**
   * @brief Declare a custom error code for this plugin.
   *
   * This function declares a custom error code that can be generated
   * by this plugin. This declaration is used to enrich the body of
   * the HTTP answer in the case of an error, and to set the proper
   * HTTP status code.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param code The error code that is internal to this plugin.
   * @param httpStatus The HTTP status corresponding to this error.
   * @param message The description of the error.
   * @return The error code that has been assigned inside the Orthanc core.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRegisterErrorCode(
    OrthancPluginContext*    context,
    int32_t                  code,
    uint16_t                 httpStatus,
    const char*              message)
  {
    OrthancPluginErrorCode target;

    _OrthancPluginRegisterErrorCode params;
    params.target = &target;
    params.code = code;
    params.httpStatus = httpStatus;
    params.message = message;

    if (context->InvokeService(context, _OrthancPluginService_RegisterErrorCode, &params) == OrthancPluginErrorCode_Success)
    {
      return target;
    }
    else
    {
      /* There was an error while assigned the error. Use a generic code. */
      return OrthancPluginErrorCode_Plugin;
    }
  }



  typedef struct
  {
    uint16_t                          group;
    uint16_t                          element;
    OrthancPluginValueRepresentation  vr;
    const char*                       name;
    uint32_t                          minMultiplicity;
    uint32_t                          maxMultiplicity;
  } _OrthancPluginRegisterDictionaryTag;
  
  /**
   * @brief Register a new tag into the DICOM dictionary.
   *
   * This function declares a new public tag in the dictionary of
   * DICOM tags that are known to Orthanc. This function should be
   * used in the OrthancPluginInitialize() callback.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param group The group of the tag.
   * @param element The element of the tag.
   * @param vr The value representation of the tag.
   * @param name The nickname of the tag.
   * @param minMultiplicity The minimum multiplicity of the tag (must be above 0).
   * @param maxMultiplicity The maximum multiplicity of the tag. A value of 0 means
   * an arbitrary multiplicity ("<tt>n</tt>").
   * @return 0 if success, other value if error.
   * @see OrthancPluginRegisterPrivateDictionaryTag()
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRegisterDictionaryTag(
    OrthancPluginContext*             context,
    uint16_t                          group,
    uint16_t                          element,
    OrthancPluginValueRepresentation  vr,
    const char*                       name,
    uint32_t                          minMultiplicity,
    uint32_t                          maxMultiplicity)
  {
    _OrthancPluginRegisterDictionaryTag params;
    params.group = group;
    params.element = element;
    params.vr = vr;
    params.name = name;
    params.minMultiplicity = minMultiplicity;
    params.maxMultiplicity = maxMultiplicity;

    return context->InvokeService(context, _OrthancPluginService_RegisterDictionaryTag, &params);
  }



  typedef struct
  {
    uint16_t                          group;
    uint16_t                          element;
    OrthancPluginValueRepresentation  vr;
    const char*                       name;
    uint32_t                          minMultiplicity;
    uint32_t                          maxMultiplicity;
    const char*                       privateCreator;
  } _OrthancPluginRegisterPrivateDictionaryTag;
  
  /**
   * @brief Register a new private tag into the DICOM dictionary.
   *
   * This function declares a new private tag in the dictionary of
   * DICOM tags that are known to Orthanc. This function should be
   * used in the OrthancPluginInitialize() callback.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param group The group of the tag.
   * @param element The element of the tag.
   * @param vr The value representation of the tag.
   * @param name The nickname of the tag.
   * @param minMultiplicity The minimum multiplicity of the tag (must be above 0).
   * @param maxMultiplicity The maximum multiplicity of the tag. A value of 0 means
   * an arbitrary multiplicity ("<tt>n</tt>").
   * @param privateCreator The private creator of this private tag.
   * @return 0 if success, other value if error.
   * @see OrthancPluginRegisterDictionaryTag()
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRegisterPrivateDictionaryTag(
    OrthancPluginContext*             context,
    uint16_t                          group,
    uint16_t                          element,
    OrthancPluginValueRepresentation  vr,
    const char*                       name,
    uint32_t                          minMultiplicity,
    uint32_t                          maxMultiplicity,
    const char*                       privateCreator)
  {
    _OrthancPluginRegisterPrivateDictionaryTag params;
    params.group = group;
    params.element = element;
    params.vr = vr;
    params.name = name;
    params.minMultiplicity = minMultiplicity;
    params.maxMultiplicity = maxMultiplicity;
    params.privateCreator = privateCreator;

    return context->InvokeService(context, _OrthancPluginService_RegisterPrivateDictionaryTag, &params);
  }



  typedef struct
  {
    OrthancPluginStorageArea*  storageArea;
    OrthancPluginResourceType  level;
  } _OrthancPluginReconstructMainDicomTags;

  /**
   * @brief Reconstruct the main DICOM tags.
   *
   * This function requests the Orthanc core to reconstruct the main
   * DICOM tags of all the resources of the given type. This function
   * can only be used as a part of the upgrade of a custom database
   * back-end. A database transaction will be automatically setup.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param storageArea The storage area.
   * @param level The type of the resources of interest.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginReconstructMainDicomTags(
    OrthancPluginContext*      context,
    OrthancPluginStorageArea*  storageArea,
    OrthancPluginResourceType  level)
  {
    _OrthancPluginReconstructMainDicomTags params;
    params.level = level;
    params.storageArea = storageArea;

    return context->InvokeService(context, _OrthancPluginService_ReconstructMainDicomTags, &params);
  }


  typedef struct
  {
    char**                          result;
    const char*                     instanceId;
    const void*                     buffer;
    uint32_t                        size;
    OrthancPluginDicomToJsonFormat  format;
    OrthancPluginDicomToJsonFlags   flags;
    uint32_t                        maxStringLength;
  } _OrthancPluginDicomToJson;


  /**
   * @brief Format a DICOM memory buffer as a JSON string.
   *
   * This function takes as input a memory buffer containing a DICOM
   * file, and outputs a JSON string representing the tags of this
   * DICOM file.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param buffer The memory buffer containing the DICOM file.
   * @param size The size of the memory buffer.
   * @param format The output format.
   * @param flags Flags governing the output.
   * @param maxStringLength The maximum length of a field. Too long fields will
   * be output as "null". The 0 value means no maximum length.
   * @return The NULL value if the case of an error, or the JSON
   * string. This string must be freed by OrthancPluginFreeString().
   * @ingroup Toolbox
   * @see OrthancPluginDicomInstanceToJson
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginDicomBufferToJson(
    OrthancPluginContext*           context,
    const void*                     buffer,
    uint32_t                        size,
    OrthancPluginDicomToJsonFormat  format,
    OrthancPluginDicomToJsonFlags   flags, 
    uint32_t                        maxStringLength)
  {
    char* result;

    _OrthancPluginDicomToJson params;
    memset(&params, 0, sizeof(params));
    params.result = &result;
    params.buffer = buffer;
    params.size = size;
    params.format = format;
    params.flags = flags;
    params.maxStringLength = maxStringLength;

    if (context->InvokeService(context, _OrthancPluginService_DicomBufferToJson, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Format a DICOM instance as a JSON string.
   *
   * This function formats a DICOM instance that is stored in Orthanc,
   * and outputs a JSON string representing the tags of this DICOM
   * instance.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instanceId The Orthanc identifier of the instance.
   * @param format The output format.
   * @param flags Flags governing the output.
   * @param maxStringLength The maximum length of a field. Too long fields will
   * be output as "null". The 0 value means no maximum length.
   * @return The NULL value if the case of an error, or the JSON
   * string. This string must be freed by OrthancPluginFreeString().
   * @ingroup Toolbox
   * @see OrthancPluginDicomInstanceToJson
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginDicomInstanceToJson(
    OrthancPluginContext*           context,
    const char*                     instanceId,
    OrthancPluginDicomToJsonFormat  format,
    OrthancPluginDicomToJsonFlags   flags, 
    uint32_t                        maxStringLength)
  {
    char* result;

    _OrthancPluginDicomToJson params;
    memset(&params, 0, sizeof(params));
    params.result = &result;
    params.instanceId = instanceId;
    params.format = format;
    params.flags = flags;
    params.maxStringLength = maxStringLength;

    if (context->InvokeService(context, _OrthancPluginService_DicomInstanceToJson, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  typedef struct
  {
    OrthancPluginMemoryBuffer*  target;
    const char*                 uri;
    uint32_t                    headersCount;
    const char* const*          headersKeys;
    const char* const*          headersValues;
    int32_t                     afterPlugins;
  } _OrthancPluginRestApiGet2;

  /**
   * @brief Make a GET call to the Orthanc REST API, with custom HTTP headers.
   * 
   * Make a GET call to the Orthanc REST API with extended
   * parameters. The result to the query is stored into a newly
   * allocated memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param uri The URI in the built-in Orthanc API.
   * @param headersCount The number of HTTP headers.
   * @param headersKeys Array containing the keys of the HTTP headers (can be <tt>NULL</tt> if no header).
   * @param headersValues Array containing the values of the HTTP headers (can be <tt>NULL</tt> if no header).
   * @param afterPlugins If 0, the built-in API of Orthanc is used.
   * If 1, the API is tainted by the plugins.
   * @return 0 if success, or the error code if failure.
   * @see OrthancPluginRestApiGet, OrthancPluginRestApiGetAfterPlugins
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginRestApiGet2(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    const char*                 uri,
    uint32_t                    headersCount,
    const char* const*          headersKeys,
    const char* const*          headersValues,
    int32_t                     afterPlugins)
  {
    _OrthancPluginRestApiGet2 params;
    params.target = target;
    params.uri = uri;
    params.headersCount = headersCount;
    params.headersKeys = headersKeys;
    params.headersValues = headersValues;
    params.afterPlugins = afterPlugins;

    return context->InvokeService(context, _OrthancPluginService_RestApiGet2, &params);
  }



  typedef struct
  {
    OrthancPluginWorklistCallback callback;
  } _OrthancPluginWorklistCallback;

  /**
   * @brief Register a callback to handle modality worklists requests.
   *
   * This function registers a callback to handle C-Find SCP requests
   * on modality worklists.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterWorklistCallback(
    OrthancPluginContext*          context,
    OrthancPluginWorklistCallback  callback)
  {
    _OrthancPluginWorklistCallback params;
    params.callback = callback;

    return context->InvokeService(context, _OrthancPluginService_RegisterWorklistCallback, &params);
  }


  
  typedef struct
  {
    OrthancPluginWorklistAnswers*      answers;
    const OrthancPluginWorklistQuery*  query;
    const void*                        dicom;
    uint32_t                           size;
  } _OrthancPluginWorklistAnswersOperation;

  /**
   * @brief Add one answer to some modality worklist request.
   *
   * This function adds one worklist (encoded as a DICOM file) to the
   * set of answers corresponding to some C-Find SCP request against
   * modality worklists.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param answers The set of answers.
   * @param query The worklist query, as received by the callback.
   * @param dicom The worklist to answer, encoded as a DICOM file.
   * @param size The size of the DICOM file.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   * @see OrthancPluginCreateDicom()
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginWorklistAddAnswer(
    OrthancPluginContext*             context,
    OrthancPluginWorklistAnswers*     answers,
    const OrthancPluginWorklistQuery* query,
    const void*                       dicom,
    uint32_t                          size)
  {
    _OrthancPluginWorklistAnswersOperation params;
    params.answers = answers;
    params.query = query;
    params.dicom = dicom;
    params.size = size;

    return context->InvokeService(context, _OrthancPluginService_WorklistAddAnswer, &params);
  }


  /**
   * @brief Mark the set of worklist answers as incomplete.
   *
   * This function marks as incomplete the set of answers
   * corresponding to some C-Find SCP request against modality
   * worklists. This must be used if canceling the handling of a
   * request when too many answers are to be returned.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param answers The set of answers.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginWorklistMarkIncomplete(
    OrthancPluginContext*          context,
    OrthancPluginWorklistAnswers*  answers)
  {
    _OrthancPluginWorklistAnswersOperation params;
    params.answers = answers;
    params.query = NULL;
    params.dicom = NULL;
    params.size = 0;

    return context->InvokeService(context, _OrthancPluginService_WorklistMarkIncomplete, &params);
  }


  typedef struct
  {
    const OrthancPluginWorklistQuery*  query;
    const void*                        dicom;
    uint32_t                           size;
    int32_t*                           isMatch;
    OrthancPluginMemoryBuffer*         target;
  } _OrthancPluginWorklistQueryOperation;

  /**
   * @brief Test whether a worklist matches the query.
   *
   * This function checks whether one worklist (encoded as a DICOM
   * file) matches the C-Find SCP query against modality
   * worklists. This function must be called before adding the
   * worklist as an answer through OrthancPluginWorklistAddAnswer().
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param query The worklist query, as received by the callback.
   * @param dicom The worklist to answer, encoded as a DICOM file.
   * @param size The size of the DICOM file.
   * @return 1 if the worklist matches the query, 0 otherwise.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE int32_t  OrthancPluginWorklistIsMatch(
    OrthancPluginContext*              context,
    const OrthancPluginWorklistQuery*  query,
    const void*                        dicom,
    uint32_t                           size)
  {
    int32_t isMatch = 0;

    _OrthancPluginWorklistQueryOperation params;
    params.query = query;
    params.dicom = dicom;
    params.size = size;
    params.isMatch = &isMatch;
    params.target = NULL;

    if (context->InvokeService(context, _OrthancPluginService_WorklistIsMatch, &params) == OrthancPluginErrorCode_Success)
    {
      return isMatch;
    }
    else
    {
      /* Error: Assume non-match */
      return 0;
    }
  }


  /**
   * @brief Retrieve the worklist query as a DICOM file.
   *
   * This function retrieves the DICOM file that underlies a C-Find
   * SCP query against modality worklists.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target Memory buffer where to store the DICOM file. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param query The worklist query, as received by the callback.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginWorklistGetDicomQuery(
    OrthancPluginContext*              context,
    OrthancPluginMemoryBuffer*         target,
    const OrthancPluginWorklistQuery*  query)
  {
    _OrthancPluginWorklistQueryOperation params;
    params.query = query;
    params.dicom = NULL;
    params.size = 0;
    params.isMatch = NULL;
    params.target = target;

    return context->InvokeService(context, _OrthancPluginService_WorklistGetDicomQuery, &params);
  }


  /**
   * @brief Get the origin of a DICOM file.
   *
   * This function returns the origin of a DICOM instance that has been received by Orthanc.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @return The origin of the instance.
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginInstanceOrigin OrthancPluginGetInstanceOrigin(
    OrthancPluginContext*             context,
    const OrthancPluginDicomInstance* instance)
  {
    OrthancPluginInstanceOrigin origin;

    _OrthancPluginAccessDicomInstance params;
    memset(&params, 0, sizeof(params));
    params.resultOrigin = &origin;
    params.instance = instance;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceOrigin, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return OrthancPluginInstanceOrigin_Unknown;
    }
    else
    {
      return origin;
    }
  }


  typedef struct
  {
    OrthancPluginMemoryBuffer*     target;
    const char*                    json;
    const OrthancPluginImage*      pixelData;
    OrthancPluginCreateDicomFlags  flags;
  } _OrthancPluginCreateDicom;

  /**
   * @brief Create a DICOM instance from a JSON string and an image.
   *
   * This function takes as input a string containing a JSON file
   * describing the content of a DICOM instance. As an output, it
   * writes the corresponding DICOM instance to a newly allocated
   * memory buffer. Additionally, an image to be encoded within the
   * DICOM instance can also be provided.
   *
   * Private tags will be associated with the private creator whose
   * value is specified in the "DefaultPrivateCreator" configuration
   * option of Orthanc. The function OrthancPluginCreateDicom2() can
   * be used if another private creator must be used to create this
   * instance.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param json The input JSON file.
   * @param pixelData The image. Can be NULL, if the pixel data is encoded inside the JSON with the data URI scheme.
   * @param flags Flags governing the output.
   * @return 0 if success, other value if error.
   * @ingroup Toolbox
   * @see OrthancPluginCreateDicom2
   * @see OrthancPluginDicomBufferToJson
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginCreateDicom(
    OrthancPluginContext*          context,
    OrthancPluginMemoryBuffer*     target,
    const char*                    json,
    const OrthancPluginImage*      pixelData,
    OrthancPluginCreateDicomFlags  flags)
  {
    _OrthancPluginCreateDicom params;
    params.target = target;
    params.json = json;
    params.pixelData = pixelData;
    params.flags = flags;

    return context->InvokeService(context, _OrthancPluginService_CreateDicom, &params);
  }


  typedef struct
  {
    OrthancPluginDecodeImageCallback callback;
  } _OrthancPluginDecodeImageCallback;

  /**
   * @brief Register a callback to handle the decoding of DICOM images.
   *
   * This function registers a custom callback to decode DICOM images,
   * extending the built-in decoder of Orthanc that uses
   * DCMTK. Starting with Orthanc 1.7.0, the exact behavior is
   * affected by the configuration option
   * "BuiltinDecoderTranscoderOrder" of Orthanc.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterDecodeImageCallback(
    OrthancPluginContext*             context,
    OrthancPluginDecodeImageCallback  callback)
  {
    _OrthancPluginDecodeImageCallback params;
    params.callback = callback;

    return context->InvokeService(context, _OrthancPluginService_RegisterDecodeImageCallback, &params);
  }
  


  typedef struct
  {
    OrthancPluginImage**       target;
    OrthancPluginPixelFormat   format;
    uint32_t                   width;
    uint32_t                   height;
    uint32_t                   pitch;
    void*                      buffer;
    const void*                constBuffer;
    uint32_t                   bufferSize;
    uint32_t                   frameIndex;
  } _OrthancPluginCreateImage;


  /**
   * @brief Create an image.
   *
   * This function creates an image of given size and format.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param format The format of the pixels.
   * @param width The width of the image.
   * @param height The height of the image.
   * @return The newly allocated image. It must be freed with OrthancPluginFreeImage().
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginImage* OrthancPluginCreateImage(
    OrthancPluginContext*     context,
    OrthancPluginPixelFormat  format,
    uint32_t                  width,
    uint32_t                  height)
  {
    OrthancPluginImage* target = NULL;

    _OrthancPluginCreateImage params;
    memset(&params, 0, sizeof(params));
    params.target = &target;
    params.format = format;
    params.width = width;
    params.height = height;

    if (context->InvokeService(context, _OrthancPluginService_CreateImage, &params) != OrthancPluginErrorCode_Success)
    {
      return NULL;
    }
    else
    {
      return target;
    }
  }


  /**
   * @brief Create an image pointing to a memory buffer.
   *
   * This function creates an image whose content points to a memory
   * buffer managed by the plugin. Note that the buffer is directly
   * accessed, no memory is allocated and no data is copied.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param format The format of the pixels.
   * @param width The width of the image.
   * @param height The height of the image.
   * @param pitch The pitch of the image (i.e. the number of bytes
   * between 2 successive lines of the image in the memory buffer).
   * @param buffer The memory buffer.
   * @return The newly allocated image. It must be freed with OrthancPluginFreeImage().
   * @ingroup Images
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginImage* OrthancPluginCreateImageAccessor(
    OrthancPluginContext*     context,
    OrthancPluginPixelFormat  format,
    uint32_t                  width,
    uint32_t                  height,
    uint32_t                  pitch,
    void*                     buffer)
  {
    OrthancPluginImage* target = NULL;

    _OrthancPluginCreateImage params;
    memset(&params, 0, sizeof(params));
    params.target = &target;
    params.format = format;
    params.width = width;
    params.height = height;
    params.pitch = pitch;
    params.buffer = buffer;

    if (context->InvokeService(context, _OrthancPluginService_CreateImageAccessor, &params) != OrthancPluginErrorCode_Success)
    {
      return NULL;
    }
    else
    {
      return target;
    }
  }



  /**
   * @brief Decode one frame from a DICOM instance.
   *
   * This function decodes one frame of a DICOM image that is stored
   * in a memory buffer. This function will give the same result as
   * OrthancPluginUncompressImage() for single-frame DICOM images.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param buffer Pointer to a memory buffer containing the DICOM image.
   * @param bufferSize Size of the memory buffer containing the DICOM image.
   * @param frameIndex The index of the frame of interest in a multi-frame image.
   * @return The uncompressed image. It must be freed with OrthancPluginFreeImage().
   * @ingroup Images
   * @see OrthancPluginGetInstanceDecodedFrame()
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginImage* OrthancPluginDecodeDicomImage(
    OrthancPluginContext*  context,
    const void*            buffer,
    uint32_t               bufferSize,
    uint32_t               frameIndex)
  {
    OrthancPluginImage* target = NULL;

    _OrthancPluginCreateImage params;
    memset(&params, 0, sizeof(params));
    params.target = &target;
    params.constBuffer = buffer;
    params.bufferSize = bufferSize;
    params.frameIndex = frameIndex;

    if (context->InvokeService(context, _OrthancPluginService_DecodeDicomImage, &params) != OrthancPluginErrorCode_Success)
    {
      return NULL;
    }
    else
    {
      return target;
    }
  }



  typedef struct
  {
    char**       result;
    const void*  buffer;
    uint32_t     size;
  } _OrthancPluginComputeHash;

  /**
   * @brief Compute an MD5 hash.
   *
   * This functions computes the MD5 cryptographic hash of the given memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param buffer The source memory buffer.
   * @param size The size in bytes of the source buffer.
   * @return The NULL value in case of error, or a string containing the cryptographic hash.
   * This string must be freed by OrthancPluginFreeString().
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginComputeMd5(
    OrthancPluginContext*  context,
    const void*            buffer,
    uint32_t               size)
  {
    char* result;

    _OrthancPluginComputeHash params;
    params.result = &result;
    params.buffer = buffer;
    params.size = size;

    if (context->InvokeService(context, _OrthancPluginService_ComputeMd5, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Compute a SHA-1 hash.
   *
   * This functions computes the SHA-1 cryptographic hash of the given memory buffer.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param buffer The source memory buffer.
   * @param size The size in bytes of the source buffer.
   * @return The NULL value in case of error, or a string containing the cryptographic hash.
   * This string must be freed by OrthancPluginFreeString().
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginComputeSha1(
    OrthancPluginContext*  context,
    const void*            buffer,
    uint32_t               size)
  {
    char* result;

    _OrthancPluginComputeHash params;
    params.result = &result;
    params.buffer = buffer;
    params.size = size;

    if (context->InvokeService(context, _OrthancPluginService_ComputeSha1, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }



  typedef struct
  {
    OrthancPluginDictionaryEntry* target;
    const char*                   name;
  } _OrthancPluginLookupDictionary;

  /**
   * @brief Get information about the given DICOM tag.
   *
   * This functions makes a lookup in the dictionary of DICOM tags
   * that are known to Orthanc, and returns information about this
   * tag. The tag can be specified using its human-readable name
   * (e.g. "PatientName") or a set of two hexadecimal numbers
   * (e.g. "0010-0020").
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target Where to store the information about the tag.
   * @param name The name of the DICOM tag.
   * @return 0 if success, other value if error.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginLookupDictionary(
    OrthancPluginContext*          context,
    OrthancPluginDictionaryEntry*  target,
    const char*                    name)
  {
    _OrthancPluginLookupDictionary params;
    params.target = target;
    params.name = name;
    return context->InvokeService(context, _OrthancPluginService_LookupDictionary, &params);
  }



  typedef struct
  {
    OrthancPluginRestOutput* output;
    const void*              answer;
    uint32_t                 answerSize;
    uint32_t                 headersCount;
    const char* const*       headersKeys;
    const char* const*       headersValues;
  } _OrthancPluginSendMultipartItem2;

  /**
   * @brief Send an item as a part of some HTTP multipart answer, with custom headers.
   *
   * This function sends an item as a part of some HTTP multipart
   * answer that was initiated by OrthancPluginStartMultipartAnswer(). In addition to
   * OrthancPluginSendMultipartItem(), this function will set HTTP header associated
   * with the item.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param answer Pointer to the memory buffer containing the item.
   * @param answerSize Number of bytes of the item.
   * @param headersCount The number of HTTP headers.
   * @param headersKeys Array containing the keys of the HTTP headers.
   * @param headersValues Array containing the values of the HTTP headers.
   * @return 0 if success, or the error code if failure (this notably happens
   * if the connection is closed by the client).
   * @see OrthancPluginSendMultipartItem()
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginSendMultipartItem2(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    const void*              answer,
    uint32_t                 answerSize,
    uint32_t                 headersCount,
    const char* const*       headersKeys,
    const char* const*       headersValues)
  {
    _OrthancPluginSendMultipartItem2 params;
    params.output = output;
    params.answer = answer;
    params.answerSize = answerSize;
    params.headersCount = headersCount;
    params.headersKeys = headersKeys;
    params.headersValues = headersValues;    

    return context->InvokeService(context, _OrthancPluginService_SendMultipartItem2, &params);
  }


  typedef struct
  {
    OrthancPluginIncomingHttpRequestFilter callback;
  } _OrthancPluginIncomingHttpRequestFilter;

  /**
   * @brief Register a callback to filter incoming HTTP requests.
   *
   * This function registers a custom callback to filter incoming HTTP/REST
   * requests received by the HTTP server of Orthanc.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   * @deprecated Please instead use OrthancPluginRegisterIncomingHttpRequestFilter2()
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterIncomingHttpRequestFilter(
    OrthancPluginContext*                   context,
    OrthancPluginIncomingHttpRequestFilter  callback)
  {
    _OrthancPluginIncomingHttpRequestFilter params;
    params.callback = callback;

    return context->InvokeService(context, _OrthancPluginService_RegisterIncomingHttpRequestFilter, &params);
  }
  


  typedef struct
  {
    OrthancPluginMemoryBuffer*  answerBody;
    OrthancPluginMemoryBuffer*  answerHeaders;
    uint16_t*                   httpStatus;
    OrthancPluginHttpMethod     method;
    const char*                 url;
    uint32_t                    headersCount;
    const char* const*          headersKeys;
    const char* const*          headersValues;
    const void*                 body;
    uint32_t                    bodySize;
    const char*                 username;
    const char*                 password;
    uint32_t                    timeout;
    const char*                 certificateFile;
    const char*                 certificateKeyFile;
    const char*                 certificateKeyPassword;
    uint8_t                     pkcs11;
  } _OrthancPluginCallHttpClient2;



  /**
   * @brief Issue a HTTP call with full flexibility.
   * 
   * Make a HTTP call to the given URL. The result to the query is
   * stored into a newly allocated memory buffer. The HTTP request
   * will be done accordingly to the global configuration of Orthanc
   * (in particular, the options "HttpProxy", "HttpTimeout",
   * "HttpsVerifyPeers", "HttpsCACertificates", and "Pkcs11" will be
   * taken into account).
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param answerBody The target memory buffer (out argument).
   *        It must be freed with OrthancPluginFreeMemoryBuffer().
   *        The value of this argument is ignored if the HTTP method is DELETE.
   * @param answerHeaders The target memory buffer for the HTTP headers in the answers (out argument). 
   *        The answer headers are formatted as a JSON object (associative array).
   *        The buffer must be freed with OrthancPluginFreeMemoryBuffer().
   *        This argument can be set to NULL if the plugin has no interest in the HTTP headers.
   * @param httpStatus The HTTP status after the execution of the request (out argument).
   * @param method HTTP method to be used.
   * @param url The URL of interest.
   * @param headersCount The number of HTTP headers.
   * @param headersKeys Array containing the keys of the HTTP headers (can be <tt>NULL</tt> if no header).
   * @param headersValues Array containing the values of the HTTP headers (can be <tt>NULL</tt> if no header).
   * @param username The username (can be <tt>NULL</tt> if no password protection).
   * @param password The password (can be <tt>NULL</tt> if no password protection).
   * @param body The HTTP body for a POST or PUT request.
   * @param bodySize The size of the body.
   * @param timeout Timeout in seconds (0 for default timeout).
   * @param certificateFile Path to the client certificate for HTTPS, in PEM format
   * (can be <tt>NULL</tt> if no client certificate or if not using HTTPS).
   * @param certificateKeyFile Path to the key of the client certificate for HTTPS, in PEM format
   * (can be <tt>NULL</tt> if no client certificate or if not using HTTPS).
   * @param certificateKeyPassword Password to unlock the key of the client certificate 
   * (can be <tt>NULL</tt> if no client certificate or if not using HTTPS).
   * @param pkcs11 Enable PKCS#11 client authentication for hardware security modules and smart cards.
   * @return 0 if success, or the error code if failure.
   * @see OrthancPluginCallPeerApi()
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginHttpClient(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  answerBody,
    OrthancPluginMemoryBuffer*  answerHeaders,
    uint16_t*                   httpStatus,
    OrthancPluginHttpMethod     method,
    const char*                 url,
    uint32_t                    headersCount,
    const char* const*          headersKeys,
    const char* const*          headersValues,
    const void*                 body,
    uint32_t                    bodySize,
    const char*                 username,
    const char*                 password,
    uint32_t                    timeout,
    const char*                 certificateFile,
    const char*                 certificateKeyFile,
    const char*                 certificateKeyPassword,
    uint8_t                     pkcs11)
  {
    _OrthancPluginCallHttpClient2 params;
    memset(&params, 0, sizeof(params));

    params.answerBody = answerBody;
    params.answerHeaders = answerHeaders;
    params.httpStatus = httpStatus;
    params.method = method;
    params.url = url;
    params.headersCount = headersCount;
    params.headersKeys = headersKeys;
    params.headersValues = headersValues;
    params.body = body;
    params.bodySize = bodySize;
    params.username = username;
    params.password = password;
    params.timeout = timeout;
    params.certificateFile = certificateFile;
    params.certificateKeyFile = certificateKeyFile;
    params.certificateKeyPassword = certificateKeyPassword;
    params.pkcs11 = pkcs11;

    return context->InvokeService(context, _OrthancPluginService_CallHttpClient2, &params);
  }


  /**
   * @brief Generate an UUID.
   *
   * Generate a random GUID/UUID (globally unique identifier).
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return NULL in the case of an error, or a newly allocated string
   * containing the UUID. This string must be freed by OrthancPluginFreeString().
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGenerateUuid(
    OrthancPluginContext*  context)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = NULL;

    if (context->InvokeService(context, _OrthancPluginService_GenerateUuid, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }




  typedef struct
  {
    OrthancPluginFindCallback callback;
  } _OrthancPluginFindCallback;

  /**
   * @brief Register a callback to handle C-Find requests.
   *
   * This function registers a callback to handle C-Find SCP requests
   * that are not related to modality worklists.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterFindCallback(
    OrthancPluginContext*      context,
    OrthancPluginFindCallback  callback)
  {
    _OrthancPluginFindCallback params;
    params.callback = callback;

    return context->InvokeService(context, _OrthancPluginService_RegisterFindCallback, &params);
  }


  typedef struct
  {
    OrthancPluginFindAnswers      *answers;
    const OrthancPluginFindQuery  *query;
    const void                    *dicom;
    uint32_t                       size;
    uint32_t                       index;
    uint32_t                      *resultUint32;
    uint16_t                      *resultGroup;
    uint16_t                      *resultElement;
    char                         **resultString;
  } _OrthancPluginFindOperation;

  /**
   * @brief Add one answer to some C-Find request.
   *
   * This function adds one answer (encoded as a DICOM file) to the
   * set of answers corresponding to some C-Find SCP request that is
   * not related to modality worklists.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param answers The set of answers.
   * @param dicom The answer to be added, encoded as a DICOM file.
   * @param size The size of the DICOM file.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   * @see OrthancPluginCreateDicom()
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginFindAddAnswer(
    OrthancPluginContext*      context,
    OrthancPluginFindAnswers*  answers,
    const void*                dicom,
    uint32_t                   size)
  {
    _OrthancPluginFindOperation params;
    memset(&params, 0, sizeof(params));
    params.answers = answers;
    params.dicom = dicom;
    params.size = size;

    return context->InvokeService(context, _OrthancPluginService_FindAddAnswer, &params);
  }


  /**
   * @brief Mark the set of C-Find answers as incomplete.
   *
   * This function marks as incomplete the set of answers
   * corresponding to some C-Find SCP request that is not related to
   * modality worklists. This must be used if canceling the handling
   * of a request when too many answers are to be returned.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param answers The set of answers.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginFindMarkIncomplete(
    OrthancPluginContext*      context,
    OrthancPluginFindAnswers*  answers)
  {
    _OrthancPluginFindOperation params;
    memset(&params, 0, sizeof(params));
    params.answers = answers;

    return context->InvokeService(context, _OrthancPluginService_FindMarkIncomplete, &params);
  }



  /**
   * @brief Get the number of tags in a C-Find query.
   *
   * This function returns the number of tags that are contained in
   * the given C-Find query.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param query The C-Find query.
   * @return The number of tags.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE uint32_t  OrthancPluginGetFindQuerySize(
    OrthancPluginContext*          context,
    const OrthancPluginFindQuery*  query)
  {
    uint32_t count = 0;

    _OrthancPluginFindOperation params;
    memset(&params, 0, sizeof(params));
    params.query = query;
    params.resultUint32 = &count;

    if (context->InvokeService(context, _OrthancPluginService_GetFindQuerySize, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return 0;
    }
    else
    {
      return count;
    }
  }


  /**
   * @brief Get one tag in a C-Find query.
   *
   * This function returns the group and the element of one DICOM tag
   * in the given C-Find query.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param group The group of the tag (output).
   * @param element The element of the tag (output).
   * @param query The C-Find query.
   * @param index The index of the tag of interest.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginGetFindQueryTag(
    OrthancPluginContext*          context,
    uint16_t*                      group,
    uint16_t*                      element,
    const OrthancPluginFindQuery*  query,
    uint32_t                       index)
  {
    _OrthancPluginFindOperation params;
    memset(&params, 0, sizeof(params));
    params.query = query;
    params.index = index;
    params.resultGroup = group;
    params.resultElement = element;

    return context->InvokeService(context, _OrthancPluginService_GetFindQueryTag, &params);
  }


  /**
   * @brief Get the symbolic name of one tag in a C-Find query.
   *
   * This function returns the symbolic name of one DICOM tag in the
   * given C-Find query.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param query The C-Find query.
   * @param index The index of the tag of interest.
   * @return The NULL value in case of error, or a string containing the name of the tag.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE char*  OrthancPluginGetFindQueryTagName(
    OrthancPluginContext*          context,
    const OrthancPluginFindQuery*  query,
    uint32_t                       index)
  {
    char* result;

    _OrthancPluginFindOperation params;
    memset(&params, 0, sizeof(params));
    params.query = query;
    params.index = index;
    params.resultString = &result;

    if (context->InvokeService(context, _OrthancPluginService_GetFindQueryTagName, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Get the value associated with one tag in a C-Find query.
   *
   * This function returns the value associated with one tag in the
   * given C-Find query.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param query The C-Find query.
   * @param index The index of the tag of interest.
   * @return The NULL value in case of error, or a string containing the value of the tag.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE char*  OrthancPluginGetFindQueryValue(
    OrthancPluginContext*          context,
    const OrthancPluginFindQuery*  query,
    uint32_t                       index)
  {
    char* result;

    _OrthancPluginFindOperation params;
    memset(&params, 0, sizeof(params));
    params.query = query;
    params.index = index;
    params.resultString = &result;

    if (context->InvokeService(context, _OrthancPluginService_GetFindQueryValue, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }
 



  typedef struct
  {
    OrthancPluginMoveCallback   callback;
    OrthancPluginGetMoveSize    getMoveSize;
    OrthancPluginApplyMove      applyMove;
    OrthancPluginFreeMove       freeMove;
  } _OrthancPluginMoveCallback;

  /**
   * @brief Register a callback to handle C-Move requests.
   *
   * This function registers a callback to handle C-Move SCP requests.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The main callback.
   * @param getMoveSize Callback to read the number of C-Move suboperations.
   * @param applyMove Callback to apply one C-Move suboperation.
   * @param freeMove Callback to free the C-Move driver.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterMoveCallback(
    OrthancPluginContext*       context,
    OrthancPluginMoveCallback   callback,
    OrthancPluginGetMoveSize    getMoveSize,
    OrthancPluginApplyMove      applyMove,
    OrthancPluginFreeMove       freeMove)
  {
    _OrthancPluginMoveCallback params;
    params.callback = callback;
    params.getMoveSize = getMoveSize;
    params.applyMove = applyMove;
    params.freeMove = freeMove;

    return context->InvokeService(context, _OrthancPluginService_RegisterMoveCallback, &params);
  }



  typedef struct
  {
    OrthancPluginFindMatcher** target;
    const void*                query;
    uint32_t                   size;
  } _OrthancPluginCreateFindMatcher;


  /**
   * @brief Create a C-Find matcher.
   *
   * This function creates a "matcher" object that can be used to
   * check whether a DICOM instance matches a C-Find query. The C-Find
   * query must be expressed as a DICOM buffer.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param query The C-Find DICOM query.
   * @param size The size of the DICOM query.
   * @return The newly allocated matcher. It must be freed with OrthancPluginFreeFindMatcher().
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginFindMatcher* OrthancPluginCreateFindMatcher(
    OrthancPluginContext*  context,
    const void*            query,
    uint32_t               size)
  {
    OrthancPluginFindMatcher* target = NULL;

    _OrthancPluginCreateFindMatcher params;
    memset(&params, 0, sizeof(params));
    params.target = &target;
    params.query = query;
    params.size = size;

    if (context->InvokeService(context, _OrthancPluginService_CreateFindMatcher, &params) != OrthancPluginErrorCode_Success)
    {
      return NULL;
    }
    else
    {
      return target;
    }
  }


  typedef struct
  {
    OrthancPluginFindMatcher*   matcher;
  } _OrthancPluginFreeFindMatcher;

  /**
   * @brief Free a C-Find matcher.
   *
   * This function frees a matcher that was created using OrthancPluginCreateFindMatcher().
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param matcher The matcher of interest.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE void  OrthancPluginFreeFindMatcher(
    OrthancPluginContext*     context, 
    OrthancPluginFindMatcher* matcher)
  {
    _OrthancPluginFreeFindMatcher params;
    params.matcher = matcher;

    context->InvokeService(context, _OrthancPluginService_FreeFindMatcher, &params);
  }


  typedef struct
  {
    const OrthancPluginFindMatcher*  matcher;
    const void*                      dicom;
    uint32_t                         size;
    int32_t*                         isMatch;
  } _OrthancPluginFindMatcherIsMatch;

  /**
   * @brief Test whether a DICOM instance matches a C-Find query.
   *
   * This function checks whether one DICOM instance matches C-Find
   * matcher that was previously allocated using
   * OrthancPluginCreateFindMatcher().
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param matcher The matcher of interest.
   * @param dicom The DICOM instance to be matched.
   * @param size The size of the DICOM instance.
   * @return 1 if the DICOM instance matches the query, 0 otherwise.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE int32_t  OrthancPluginFindMatcherIsMatch(
    OrthancPluginContext*            context,
    const OrthancPluginFindMatcher*  matcher,
    const void*                      dicom,
    uint32_t                         size)
  {
    int32_t isMatch = 0;

    _OrthancPluginFindMatcherIsMatch params;
    params.matcher = matcher;
    params.dicom = dicom;
    params.size = size;
    params.isMatch = &isMatch;

    if (context->InvokeService(context, _OrthancPluginService_FindMatcherIsMatch, &params) == OrthancPluginErrorCode_Success)
    {
      return isMatch;
    }
    else
    {
      /* Error: Assume non-match */
      return 0;
    }
  }


  typedef struct
  {
    OrthancPluginIncomingHttpRequestFilter2 callback;
  } _OrthancPluginIncomingHttpRequestFilter2;

  /**
   * @brief Register a callback to filter incoming HTTP requests.
   *
   * This function registers a custom callback to filter incoming HTTP/REST
   * requests received by the HTTP server of Orthanc.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterIncomingHttpRequestFilter2(
    OrthancPluginContext*                   context,
    OrthancPluginIncomingHttpRequestFilter2 callback)
  {
    _OrthancPluginIncomingHttpRequestFilter2 params;
    params.callback = callback;

    return context->InvokeService(context, _OrthancPluginService_RegisterIncomingHttpRequestFilter2, &params);
  }



  typedef struct
  {
    OrthancPluginPeers**  peers;
  } _OrthancPluginGetPeers;

  /**
   * @brief Return the list of available Orthanc peers.
   *
   * This function returns the parameters of the Orthanc peers that are known to
   * the Orthanc server hosting the plugin.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return NULL if error, or a newly allocated opaque data structure containing the peers.
   * This structure must be freed with OrthancPluginFreePeers().
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginPeers* OrthancPluginGetPeers(
    OrthancPluginContext*  context)
  {
    OrthancPluginPeers* peers = NULL;

    _OrthancPluginGetPeers params;
    memset(&params, 0, sizeof(params));
    params.peers = &peers;

    if (context->InvokeService(context, _OrthancPluginService_GetPeers, &params) != OrthancPluginErrorCode_Success)
    {
      return NULL;
    }
    else
    {
      return peers;
    }
  }


  typedef struct
  {
    OrthancPluginPeers*   peers;
  } _OrthancPluginFreePeers;

  /**
   * @brief Free the list of available Orthanc peers.
   *
   * This function frees the data structure returned by OrthancPluginGetPeers().
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param peers The data structure describing the Orthanc peers.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE void  OrthancPluginFreePeers(
    OrthancPluginContext*     context, 
    OrthancPluginPeers* peers)
  {
    _OrthancPluginFreePeers params;
    params.peers = peers;

    context->InvokeService(context, _OrthancPluginService_FreePeers, &params);
  }


  typedef struct
  {
    uint32_t*                  target;
    const OrthancPluginPeers*  peers;
  } _OrthancPluginGetPeersCount;

  /**
   * @brief Get the number of Orthanc peers.
   *
   * This function returns the number of Orthanc peers.
   *
   * This function is thread-safe: Several threads sharing the same
   * OrthancPluginPeers object can simultaneously call this function.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param peers The data structure describing the Orthanc peers.
   * @result The number of peers. 
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE uint32_t OrthancPluginGetPeersCount(
    OrthancPluginContext*      context,
    const OrthancPluginPeers*  peers)
  {
    uint32_t target = 0;

    _OrthancPluginGetPeersCount params;
    memset(&params, 0, sizeof(params));
    params.target = &target;
    params.peers = peers;

    if (context->InvokeService(context, _OrthancPluginService_GetPeersCount, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return 0;
    }
    else
    {
      return target;
    }
  }


  typedef struct
  {
    const char**               target;
    const OrthancPluginPeers*  peers;
    uint32_t                   peerIndex;
    const char*                userProperty;
  } _OrthancPluginGetPeerProperty;

  /**
   * @brief Get the symbolic name of an Orthanc peer.
   *
   * This function returns the symbolic name of the Orthanc peer,
   * which corresponds to the key of the "OrthancPeers" configuration
   * option of Orthanc.
   *
   * This function is thread-safe: Several threads sharing the same
   * OrthancPluginPeers object can simultaneously call this function.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param peers The data structure describing the Orthanc peers.
   * @param peerIndex The index of the peer of interest.
   * This value must be lower than OrthancPluginGetPeersCount().
   * @result The symbolic name, or NULL in the case of an error.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE const char* OrthancPluginGetPeerName(
    OrthancPluginContext*      context,
    const OrthancPluginPeers*  peers,
    uint32_t                   peerIndex)
  {
    const char* target = NULL;

    _OrthancPluginGetPeerProperty params;
    memset(&params, 0, sizeof(params));
    params.target = &target;
    params.peers = peers;
    params.peerIndex = peerIndex;
    params.userProperty = NULL;

    if (context->InvokeService(context, _OrthancPluginService_GetPeerName, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }


  /**
   * @brief Get the base URL of an Orthanc peer.
   *
   * This function returns the base URL to the REST API of some Orthanc peer.
   *
   * This function is thread-safe: Several threads sharing the same
   * OrthancPluginPeers object can simultaneously call this function.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param peers The data structure describing the Orthanc peers.
   * @param peerIndex The index of the peer of interest.
   * This value must be lower than OrthancPluginGetPeersCount().
   * @result The URL, or NULL in the case of an error.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE const char* OrthancPluginGetPeerUrl(
    OrthancPluginContext*      context,
    const OrthancPluginPeers*  peers,
    uint32_t                   peerIndex)
  {
    const char* target = NULL;

    _OrthancPluginGetPeerProperty params;
    memset(&params, 0, sizeof(params));
    params.target = &target;
    params.peers = peers;
    params.peerIndex = peerIndex;
    params.userProperty = NULL;

    if (context->InvokeService(context, _OrthancPluginService_GetPeerUrl, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }



  /**
   * @brief Get some user-defined property of an Orthanc peer.
   *
   * This function returns some user-defined property of some Orthanc
   * peer. An user-defined property is a property that is associated
   * with the peer in the Orthanc configuration file, but that is not
   * recognized by the Orthanc core.
   *
   * This function is thread-safe: Several threads sharing the same
   * OrthancPluginPeers object can simultaneously call this function.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param peers The data structure describing the Orthanc peers.
   * @param peerIndex The index of the peer of interest.
   * This value must be lower than OrthancPluginGetPeersCount().
   * @param userProperty The user property of interest.
   * @result The value of the user property, or NULL if it is not defined.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE const char* OrthancPluginGetPeerUserProperty(
    OrthancPluginContext*      context,
    const OrthancPluginPeers*  peers,
    uint32_t                   peerIndex,
    const char*                userProperty)
  {
    const char* target = NULL;

    _OrthancPluginGetPeerProperty params;
    memset(&params, 0, sizeof(params));
    params.target = &target;
    params.peers = peers;
    params.peerIndex = peerIndex;
    params.userProperty = userProperty;

    if (context->InvokeService(context, _OrthancPluginService_GetPeerUserProperty, &params) != OrthancPluginErrorCode_Success)
    {
      /* No such user property */
      return NULL;
    }
    else
    {
      return target;
    }
  }



  typedef struct
  {
    OrthancPluginMemoryBuffer*  answerBody;
    OrthancPluginMemoryBuffer*  answerHeaders;
    uint16_t*                   httpStatus;
    const OrthancPluginPeers*   peers;
    uint32_t                    peerIndex;
    OrthancPluginHttpMethod     method;
    const char*                 uri;
    uint32_t                    additionalHeadersCount;
    const char* const*          additionalHeadersKeys;
    const char* const*          additionalHeadersValues;
    const void*                 body;
    uint32_t                    bodySize;
    uint32_t                    timeout;
  } _OrthancPluginCallPeerApi;

  /**
   * @brief Call the REST API of an Orthanc peer.
   * 
   * Make a REST call to the given URI in the REST API of a remote
   * Orthanc peer. The result to the query is stored into a newly
   * allocated memory buffer. The HTTP request will be done according
   * to the "OrthancPeers" configuration option of Orthanc.
   *
   * This function is thread-safe: Several threads sharing the same
   * OrthancPluginPeers object can simultaneously call this function.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param answerBody The target memory buffer (out argument).
   *        It must be freed with OrthancPluginFreeMemoryBuffer().
   *        The value of this argument is ignored if the HTTP method is DELETE.
   * @param answerHeaders The target memory buffer for the HTTP headers in the answers (out argument). 
   *        The answer headers are formatted as a JSON object (associative array).
   *        The buffer must be freed with OrthancPluginFreeMemoryBuffer().
   *        This argument can be set to NULL if the plugin has no interest in the HTTP headers.
   * @param httpStatus The HTTP status after the execution of the request (out argument).
   * @param peers The data structure describing the Orthanc peers.
   * @param peerIndex The index of the peer of interest.
   * This value must be lower than OrthancPluginGetPeersCount().
   * @param method HTTP method to be used.
   * @param uri The URI of interest in the REST API.
   * @param additionalHeadersCount The number of HTTP headers to be added to the
   * HTTP headers provided in the global configuration of Orthanc.
   * @param additionalHeadersKeys Array containing the keys of the HTTP headers (can be <tt>NULL</tt> if no header).
   * @param additionalHeadersValues Array containing the values of the HTTP headers (can be <tt>NULL</tt> if no header).
   * @param body The HTTP body for a POST or PUT request.
   * @param bodySize The size of the body.
   * @param timeout Timeout in seconds (0 for default timeout).
   * @return 0 if success, or the error code if failure.
   * @see OrthancPluginHttpClient()
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginCallPeerApi(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  answerBody,
    OrthancPluginMemoryBuffer*  answerHeaders,
    uint16_t*                   httpStatus,
    const OrthancPluginPeers*   peers,
    uint32_t                    peerIndex,
    OrthancPluginHttpMethod     method,
    const char*                 uri,
    uint32_t                    additionalHeadersCount,
    const char* const*          additionalHeadersKeys,
    const char* const*          additionalHeadersValues,
    const void*                 body,
    uint32_t                    bodySize,
    uint32_t                    timeout)
  {
    _OrthancPluginCallPeerApi params;
    memset(&params, 0, sizeof(params));

    params.answerBody = answerBody;
    params.answerHeaders = answerHeaders;
    params.httpStatus = httpStatus;
    params.peers = peers;
    params.peerIndex = peerIndex;
    params.method = method;
    params.uri = uri;
    params.additionalHeadersCount = additionalHeadersCount;
    params.additionalHeadersKeys = additionalHeadersKeys;
    params.additionalHeadersValues = additionalHeadersValues;
    params.body = body;
    params.bodySize = bodySize;
    params.timeout = timeout;

    return context->InvokeService(context, _OrthancPluginService_CallPeerApi, &params);
  }





  typedef struct
  {
    OrthancPluginJob**              target;
    void                           *job;
    OrthancPluginJobFinalize        finalize;
    const char                     *type;
    OrthancPluginJobGetProgress     getProgress;
    OrthancPluginJobGetContent      getContent;
    OrthancPluginJobGetSerialized   getSerialized;
    OrthancPluginJobStep            step;
    OrthancPluginJobStop            stop;
    OrthancPluginJobReset           reset;
  } _OrthancPluginCreateJob;

  /**
   * @brief Create a custom job.
   *
   * This function creates a custom job to be run by the jobs engine
   * of Orthanc.
   * 
   * Orthanc starts one dedicated thread per custom job that is
   * running. It is guaranteed that all the callbacks will only be
   * called from this single dedicated thread, in mutual exclusion: As
   * a consequence, it is *not* mandatory to protect the various
   * callbacks by mutexes.
   * 
   * The custom job can nonetheless launch its own processing threads
   * on the first call to the "step()" callback, and stop them once
   * the "stop()" callback is called.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param job The job to be executed.
   * @param finalize The finalization callback.
   * @param type The type of the job, provided to the job unserializer. 
   * See OrthancPluginRegisterJobsUnserializer().
   * @param getProgress The progress callback.
   * @param getContent The content callback.
   * @param getSerialized The serialization callback.
   * @param step The callback to execute the individual steps of the job.
   * @param stop The callback that is invoked once the job leaves the "running" state.
   * @param reset The callback that is invoked if a stopped job is started again.
   * @return The newly allocated job. It must be freed with OrthancPluginFreeJob(),
   * as long as it is not submitted with OrthancPluginSubmitJob().
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginJob *OrthancPluginCreateJob(
    OrthancPluginContext           *context,
    void                           *job,
    OrthancPluginJobFinalize        finalize,
    const char                     *type,
    OrthancPluginJobGetProgress     getProgress,
    OrthancPluginJobGetContent      getContent,
    OrthancPluginJobGetSerialized   getSerialized,
    OrthancPluginJobStep            step,
    OrthancPluginJobStop            stop,
    OrthancPluginJobReset           reset)
  {
    OrthancPluginJob* target = NULL;

    _OrthancPluginCreateJob params;
    memset(&params, 0, sizeof(params));

    params.target = &target;
    params.job = job;
    params.finalize = finalize;
    params.type = type;
    params.getProgress = getProgress;
    params.getContent = getContent;
    params.getSerialized = getSerialized;
    params.step = step;
    params.stop = stop;
    params.reset = reset;

    if (context->InvokeService(context, _OrthancPluginService_CreateJob, &params) != OrthancPluginErrorCode_Success ||
        target == NULL)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }


  typedef struct
  {
    OrthancPluginJob*   job;
  } _OrthancPluginFreeJob;

  /**
   * @brief Free a custom job.
   *
   * This function frees an image that was created with OrthancPluginCreateJob().
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param job The job.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE void  OrthancPluginFreeJob(
    OrthancPluginContext* context, 
    OrthancPluginJob*     job)
  {
    _OrthancPluginFreeJob params;
    params.job = job;

    context->InvokeService(context, _OrthancPluginService_FreeJob, &params);
  }


  
  typedef struct
  {
    char**             resultId;
    OrthancPluginJob  *job;
    int                priority;
  } _OrthancPluginSubmitJob;

  /**
   * @brief Submit a new job to the jobs engine of Orthanc.
   *
   * This function adds the given job to the pending jobs of
   * Orthanc. Orthanc will take take of freeing it by invoking the
   * finalization callback provided to OrthancPluginCreateJob().
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param job The job, as received by OrthancPluginCreateJob().
   * @param priority The priority of the job.
   * @return ID of the newly-submitted job. This string must be freed by OrthancPluginFreeString().
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE char *OrthancPluginSubmitJob(
    OrthancPluginContext   *context,
    OrthancPluginJob       *job,
    int                     priority)
  {
    char* resultId = NULL;

    _OrthancPluginSubmitJob params;
    memset(&params, 0, sizeof(params));

    params.resultId = &resultId;
    params.job = job;
    params.priority = priority;

    if (context->InvokeService(context, _OrthancPluginService_SubmitJob, &params) != OrthancPluginErrorCode_Success ||
        resultId == NULL)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return resultId;
    }
  }
  


  typedef struct
  {
    OrthancPluginJobsUnserializer unserializer;
  } _OrthancPluginJobsUnserializer;

  /**
   * @brief Register an unserializer for custom jobs.
   *
   * This function registers an unserializer that decodes custom jobs
   * from a JSON string. This callback is invoked when the jobs engine
   * of Orthanc is started (on Orthanc initialization), for each job
   * that is stored in the Orthanc database.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param unserializer The job unserializer.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginRegisterJobsUnserializer(
    OrthancPluginContext*          context,
    OrthancPluginJobsUnserializer  unserializer)
  {
    _OrthancPluginJobsUnserializer params;
    params.unserializer = unserializer;

    context->InvokeService(context, _OrthancPluginService_RegisterJobsUnserializer, &params);
  }



  typedef struct
  {
    OrthancPluginRestOutput* output;
    const char*              details;
    uint8_t                  log;
  } _OrthancPluginSetHttpErrorDetails;

  /**
   * @brief Provide a detailed description for an HTTP error.
   *
   * This function sets the detailed description associated with an
   * HTTP error. This description will be displayed in the "Details"
   * field of the JSON body of the HTTP answer. It is only taken into
   * consideration if the REST callback returns an error code that is
   * different from "OrthancPluginErrorCode_Success", and if the
   * "HttpDescribeErrors" configuration option of Orthanc is set to
   * "true".
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param output The HTTP connection to the client application.
   * @param details The details of the error message.
   * @param log Whether to also write the detailed error to the Orthanc logs.
   * @ingroup REST
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginSetHttpErrorDetails(
    OrthancPluginContext*    context,
    OrthancPluginRestOutput* output,
    const char*              details,
    uint8_t                  log)
  {
    _OrthancPluginSetHttpErrorDetails params;
    params.output = output;
    params.details = details;
    params.log = log;
    context->InvokeService(context, _OrthancPluginService_SetHttpErrorDetails, &params);
  }



  typedef struct
  {
    const char** result;
    const char*  argument;
  } _OrthancPluginRetrieveStaticString;

  /**
   * @brief Detect the MIME type of a file.
   *
   * This function returns the MIME type of a file by inspecting its extension.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param path Path to the file.
   * @return The MIME type. This is a statically-allocated
   * string, do not free it.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE const char* OrthancPluginAutodetectMimeType(
    OrthancPluginContext*  context,
    const char*            path)
  {
    const char* result = NULL;

    _OrthancPluginRetrieveStaticString params;
    params.result = &result;
    params.argument = path;

    if (context->InvokeService(context, _OrthancPluginService_AutodetectMimeType, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }



  typedef struct
  {
    const char*               name;
    float                     value;
    OrthancPluginMetricsType  type;
  } _OrthancPluginSetMetricsValue;

  /**
   * @brief Set the value of a metrics.
   *
   * This function sets the value of a metrics to monitor the behavior
   * of the plugin through tools such as Prometheus. The values of all
   * the metrics are stored within the Orthanc context.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param name The name of the metrics to be set.
   * @param value The value of the metrics.
   * @param type The type of the metrics. This parameter is only taken into consideration
   * the first time this metrics is set.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginSetMetricsValue(
    OrthancPluginContext*     context,
    const char*               name,
    float                     value,
    OrthancPluginMetricsType  type)
  {
    _OrthancPluginSetMetricsValue params;
    params.name = name;
    params.value = value;
    params.type = type;
    context->InvokeService(context, _OrthancPluginService_SetMetricsValue, &params);
  }



  typedef struct
  {
    OrthancPluginRefreshMetricsCallback  callback;
  } _OrthancPluginRegisterRefreshMetricsCallback;

  /**
   * @brief Register a callback to refresh the metrics.
   *
   * This function registers a callback to refresh the metrics. The
   * callback must make calls to OrthancPluginSetMetricsValue().
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback function to handle the refresh.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginRegisterRefreshMetricsCallback(
    OrthancPluginContext*               context,
    OrthancPluginRefreshMetricsCallback callback)
  {
    _OrthancPluginRegisterRefreshMetricsCallback params;
    params.callback = callback;
    context->InvokeService(context, _OrthancPluginService_RegisterRefreshMetricsCallback, &params);
  }




  typedef struct
  {
    char**                               target;
    const void*                          dicom;
    uint32_t                             dicomSize;
    OrthancPluginDicomWebBinaryCallback  callback;
  } _OrthancPluginEncodeDicomWeb;

  /**
   * @brief Convert a DICOM instance to DICOMweb JSON.
   *
   * This function converts a memory buffer containing a DICOM instance,
   * into its DICOMweb JSON representation.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param dicom Pointer to the DICOM instance.
   * @param dicomSize Size of the DICOM instance.
   * @param callback Callback to set the value of the binary tags.
   * @see OrthancPluginCreateDicom()
   * @return The NULL value in case of error, or the JSON document. This string must
   * be freed by OrthancPluginFreeString().
   * @deprecated OrthancPluginEncodeDicomWebJson2()
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginEncodeDicomWebJson(
    OrthancPluginContext*                context,
    const void*                          dicom,
    uint32_t                             dicomSize,
    OrthancPluginDicomWebBinaryCallback  callback)
  {
    char* target = NULL;
    
    _OrthancPluginEncodeDicomWeb params;
    params.target = &target;
    params.dicom = dicom;
    params.dicomSize = dicomSize;
    params.callback = callback;

    if (context->InvokeService(context, _OrthancPluginService_EncodeDicomWebJson, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }


  /**
   * @brief Convert a DICOM instance to DICOMweb XML.
   *
   * This function converts a memory buffer containing a DICOM instance,
   * into its DICOMweb XML representation.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param dicom Pointer to the DICOM instance.
   * @param dicomSize Size of the DICOM instance.
   * @param callback Callback to set the value of the binary tags.
   * @return The NULL value in case of error, or the XML document. This string must
   * be freed by OrthancPluginFreeString().
   * @see OrthancPluginCreateDicom()
   * @deprecated OrthancPluginEncodeDicomWebXml2()
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginEncodeDicomWebXml(
    OrthancPluginContext*                context,
    const void*                          dicom,
    uint32_t                             dicomSize,
    OrthancPluginDicomWebBinaryCallback  callback)
  {
    char* target = NULL;
    
    _OrthancPluginEncodeDicomWeb params;
    params.target = &target;
    params.dicom = dicom;
    params.dicomSize = dicomSize;
    params.callback = callback;

    if (context->InvokeService(context, _OrthancPluginService_EncodeDicomWebXml, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }
  


  typedef struct
  {
    char**                                target;
    const void*                           dicom;
    uint32_t                              dicomSize;
    OrthancPluginDicomWebBinaryCallback2  callback;
    void*                                 payload;
  } _OrthancPluginEncodeDicomWeb2;

  /**
   * @brief Convert a DICOM instance to DICOMweb JSON.
   *
   * This function converts a memory buffer containing a DICOM instance,
   * into its DICOMweb JSON representation.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param dicom Pointer to the DICOM instance.
   * @param dicomSize Size of the DICOM instance.
   * @param callback Callback to set the value of the binary tags.
   * @param payload User payload.
   * @return The NULL value in case of error, or the JSON document. This string must
   * be freed by OrthancPluginFreeString().
   * @see OrthancPluginCreateDicom()
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginEncodeDicomWebJson2(
    OrthancPluginContext*                 context,
    const void*                           dicom,
    uint32_t                              dicomSize,
    OrthancPluginDicomWebBinaryCallback2  callback,
    void*                                 payload)
  {
    char* target = NULL;
    
    _OrthancPluginEncodeDicomWeb2 params;
    params.target = &target;
    params.dicom = dicom;
    params.dicomSize = dicomSize;
    params.callback = callback;
    params.payload = payload;

    if (context->InvokeService(context, _OrthancPluginService_EncodeDicomWebJson2, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }


  /**
   * @brief Convert a DICOM instance to DICOMweb XML.
   *
   * This function converts a memory buffer containing a DICOM instance,
   * into its DICOMweb XML representation.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param dicom Pointer to the DICOM instance.
   * @param dicomSize Size of the DICOM instance.
   * @param callback Callback to set the value of the binary tags.
   * @param payload User payload.
   * @return The NULL value in case of error, or the XML document. This string must
   * be freed by OrthancPluginFreeString().
   * @see OrthancPluginCreateDicom()
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginEncodeDicomWebXml2(
    OrthancPluginContext*                 context,
    const void*                           dicom,
    uint32_t                              dicomSize,
    OrthancPluginDicomWebBinaryCallback2  callback,
    void*                                 payload)
  {
    char* target = NULL;
    
    _OrthancPluginEncodeDicomWeb2 params;
    params.target = &target;
    params.dicom = dicom;
    params.dicomSize = dicomSize;
    params.callback = callback;
    params.payload = payload;

    if (context->InvokeService(context, _OrthancPluginService_EncodeDicomWebXml2, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }
  


  /**
   * @brief Callback executed when a HTTP header is received during a chunked transfer.
   *
   * Signature of a callback function that is called by Orthanc acting
   * as a HTTP client during a chunked HTTP transfer, as soon as it
   * receives one HTTP header from the answer of the remote HTTP
   * server.
   *
   * @see OrthancPluginChunkedHttpClient()
   * @param answer The user payload, as provided by the calling plugin.
   * @param key The key of the HTTP header.
   * @param value The value of the HTTP header.
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginChunkedClientAnswerAddHeader) (
    void* answer,
    const char* key,
    const char* value);


  /**
   * @brief Callback executed when an answer chunk is received during a chunked transfer.
   *
   * Signature of a callback function that is called by Orthanc acting
   * as a HTTP client during a chunked HTTP transfer, as soon as it
   * receives one data chunk from the answer of the remote HTTP
   * server.
   *
   * @see OrthancPluginChunkedHttpClient()
   * @param answer The user payload, as provided by the calling plugin.
   * @param data The content of the data chunk.
   * @param size The size of the data chunk.
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginChunkedClientAnswerAddChunk) (
    void* answer,
    const void* data,
    uint32_t size);
  

  /**
   * @brief Callback to know whether the request body is entirely read during a chunked transfer 
   *
   * Signature of a callback function that is called by Orthanc acting
   * as a HTTP client during a chunked HTTP transfer, while reading
   * the body of a POST or PUT request. The plugin must answer "1" as
   * soon as the body is entirely read: The "request" data structure
   * must act as an iterator.
   *
   * @see OrthancPluginChunkedHttpClient()
   * @param request The user payload, as provided by the calling plugin.
   * @return "1" if the body is over, or "0" if there is still data to be read.
   * @ingroup Toolbox
   **/
  typedef uint8_t (*OrthancPluginChunkedClientRequestIsDone) (void* request);


  /**
   * @brief Callback to advance in the request body during a chunked transfer 
   *
   * Signature of a callback function that is called by Orthanc acting
   * as a HTTP client during a chunked HTTP transfer, while reading
   * the body of a POST or PUT request. This function asks the plugin
   * to advance to the next chunk of data of the request body: The
   * "request" data structure must act as an iterator.
   *
   * @see OrthancPluginChunkedHttpClient()
   * @param request The user payload, as provided by the calling plugin.
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginChunkedClientRequestNext) (void* request);


  /**
   * @brief Callback to read the current chunk of the request body during a chunked transfer 
   *
   * Signature of a callback function that is called by Orthanc acting
   * as a HTTP client during a chunked HTTP transfer, while reading
   * the body of a POST or PUT request. The plugin must provide the
   * content of the current chunk of data of the request body.
   *
   * @see OrthancPluginChunkedHttpClient()
   * @param request The user payload, as provided by the calling plugin.
   * @return The content of the current request chunk.
   * @ingroup Toolbox
   **/
  typedef const void* (*OrthancPluginChunkedClientRequestGetChunkData) (void* request);


  /**
   * @brief Callback to read the size of the current request chunk during a chunked transfer 
   *
   * Signature of a callback function that is called by Orthanc acting
   * as a HTTP client during a chunked HTTP transfer, while reading
   * the body of a POST or PUT request. The plugin must provide the
   * size of the current chunk of data of the request body.
   *
   * @see OrthancPluginChunkedHttpClient()
   * @param request The user payload, as provided by the calling plugin.
   * @return The size of the current request chunk.
   * @ingroup Toolbox
   **/
  typedef uint32_t (*OrthancPluginChunkedClientRequestGetChunkSize) (void* request);

  
  typedef struct
  {
    void*                                          answer;
    OrthancPluginChunkedClientAnswerAddChunk       answerAddChunk;
    OrthancPluginChunkedClientAnswerAddHeader      answerAddHeader;
    uint16_t*                                      httpStatus;
    OrthancPluginHttpMethod                        method;
    const char*                                    url;
    uint32_t                                       headersCount;
    const char* const*                             headersKeys;
    const char* const*                             headersValues;
    void*                                          request;
    OrthancPluginChunkedClientRequestIsDone        requestIsDone;
    OrthancPluginChunkedClientRequestGetChunkData  requestChunkData;
    OrthancPluginChunkedClientRequestGetChunkSize  requestChunkSize;
    OrthancPluginChunkedClientRequestNext          requestNext;
    const char*                                    username;
    const char*                                    password;
    uint32_t                                       timeout;
    const char*                                    certificateFile;
    const char*                                    certificateKeyFile;
    const char*                                    certificateKeyPassword;
    uint8_t                                        pkcs11;
  } _OrthancPluginChunkedHttpClient;

  
  /**
   * @brief Issue a HTTP call, using chunked HTTP transfers.
   * 
   * Make a HTTP call to the given URL using chunked HTTP
   * transfers. The request body is provided as an iterator over data
   * chunks. The answer is provided as a sequence of function calls
   * with the individual HTTP headers and answer chunks.
   * 
   * Contrarily to OrthancPluginHttpClient() that entirely stores the
   * request body and the answer body in memory buffers, this function
   * uses chunked HTTP transfers. This results in a lower memory
   * consumption. Pay attention to the fact that Orthanc servers with
   * version <= 1.5.6 do not support chunked transfers: You must use
   * OrthancPluginHttpClient() if contacting such older servers.
   *
   * The HTTP request will be done accordingly to the global
   * configuration of Orthanc (in particular, the options "HttpProxy",
   * "HttpTimeout", "HttpsVerifyPeers", "HttpsCACertificates", and
   * "Pkcs11" will be taken into account).
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param answer The user payload for the answer body. It will be provided to the callbacks for the answer.
   * @param answerAddChunk Callback function to report a data chunk from the answer body.
   * @param answerAddHeader Callback function to report an HTTP header sent by the remote server.
   * @param httpStatus The HTTP status after the execution of the request (out argument).
   * @param method HTTP method to be used.
   * @param url The URL of interest.
   * @param headersCount The number of HTTP headers.
   * @param headersKeys Array containing the keys of the HTTP headers (can be <tt>NULL</tt> if no header).
   * @param headersValues Array containing the values of the HTTP headers (can be <tt>NULL</tt> if no header).
   * @param request The user payload containing the request body, and acting as an iterator.
   * It will be provided to the callbacks for the request.
   * @param requestIsDone Callback function to tell whether the request body is entirely read.
   * @param requestChunkData Callback function to get the content of the current data chunk of the request body.
   * @param requestChunkSize Callback function to get the size of the current data chunk of the request body.
   * @param requestNext Callback function to advance to the next data chunk of the request body.
   * @param username The username (can be <tt>NULL</tt> if no password protection).
   * @param password The password (can be <tt>NULL</tt> if no password protection).
   * @param timeout Timeout in seconds (0 for default timeout).
   * @param certificateFile Path to the client certificate for HTTPS, in PEM format
   * (can be <tt>NULL</tt> if no client certificate or if not using HTTPS).
   * @param certificateKeyFile Path to the key of the client certificate for HTTPS, in PEM format
   * (can be <tt>NULL</tt> if no client certificate or if not using HTTPS).
   * @param certificateKeyPassword Password to unlock the key of the client certificate 
   * (can be <tt>NULL</tt> if no client certificate or if not using HTTPS).
   * @param pkcs11 Enable PKCS#11 client authentication for hardware security modules and smart cards.
   * @return 0 if success, or the error code if failure.
   * @see OrthancPluginHttpClient()
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginChunkedHttpClient(
    OrthancPluginContext*                          context,
    void*                                          answer,
    OrthancPluginChunkedClientAnswerAddChunk       answerAddChunk,
    OrthancPluginChunkedClientAnswerAddHeader      answerAddHeader,
    uint16_t*                                      httpStatus,
    OrthancPluginHttpMethod                        method,
    const char*                                    url,
    uint32_t                                       headersCount,
    const char* const*                             headersKeys,
    const char* const*                             headersValues,
    void*                                          request,
    OrthancPluginChunkedClientRequestIsDone        requestIsDone,
    OrthancPluginChunkedClientRequestGetChunkData  requestChunkData,
    OrthancPluginChunkedClientRequestGetChunkSize  requestChunkSize,
    OrthancPluginChunkedClientRequestNext          requestNext,
    const char*                                    username,
    const char*                                    password,
    uint32_t                                       timeout,
    const char*                                    certificateFile,
    const char*                                    certificateKeyFile,
    const char*                                    certificateKeyPassword,
    uint8_t                                        pkcs11)
  {
    _OrthancPluginChunkedHttpClient params;
    memset(&params, 0, sizeof(params));

    /* In common with OrthancPluginHttpClient() */
    params.httpStatus = httpStatus;
    params.method = method;
    params.url = url;
    params.headersCount = headersCount;
    params.headersKeys = headersKeys;
    params.headersValues = headersValues;
    params.username = username;
    params.password = password;
    params.timeout = timeout;
    params.certificateFile = certificateFile;
    params.certificateKeyFile = certificateKeyFile;
    params.certificateKeyPassword = certificateKeyPassword;
    params.pkcs11 = pkcs11;

    /* For chunked body/answer */
    params.answer = answer;
    params.answerAddChunk = answerAddChunk;
    params.answerAddHeader = answerAddHeader;
    params.request = request;
    params.requestIsDone = requestIsDone;
    params.requestChunkData = requestChunkData;
    params.requestChunkSize = requestChunkSize;
    params.requestNext = requestNext;

    return context->InvokeService(context, _OrthancPluginService_ChunkedHttpClient, &params);
  }



  /**
   * @brief Opaque structure that reads the content of a HTTP request body during a chunked HTTP transfer.
   * @ingroup Callbacks
   **/
  typedef struct _OrthancPluginServerChunkedRequestReader_t OrthancPluginServerChunkedRequestReader;



  /**
   * @brief Callback to create a reader to handle incoming chunked HTTP transfers.
   *
   * Signature of a callback function that is called by Orthanc acting
   * as a HTTP server that supports chunked HTTP transfers. This
   * callback is only invoked if the HTTP method is POST or PUT. The
   * callback must create an user-specific "reader" object that will
   * be fed with the body of the incoming body.
   * 
   * @see OrthancPluginRegisterChunkedRestCallback()
   * @param reader Memory location that must be filled with the newly-created reader.
   * @param url The URI that is accessed.
   * @param request The body of the HTTP request. Note that "body" and "bodySize" are not used.
   * @return 0 if success, or the error code if failure.
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginServerChunkedRequestReaderFactory) (
    OrthancPluginServerChunkedRequestReader**  reader,
    const char*                                url,
    const OrthancPluginHttpRequest*            request);

  
  /**
   * @brief Callback invoked whenever a new data chunk is available during a chunked transfer.
   *
   * Signature of a callback function that is called by Orthanc acting
   * as a HTTP server that supports chunked HTTP transfers. This callback
   * is invoked as soon as a new data chunk is available for the request body.
   * 
   * @see OrthancPluginRegisterChunkedRestCallback()
   * @param reader The user payload, as created by the OrthancPluginServerChunkedRequestReaderFactory() callback.
   * @param data The content of the data chunk.
   * @param size The size of the data chunk.
   * @return 0 if success, or the error code if failure.
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginServerChunkedRequestReaderAddChunk) (
    OrthancPluginServerChunkedRequestReader* reader,
    const void*                              data,
    uint32_t                                 size);
    

  /**
   * @brief Callback invoked whenever the request body is entirely received.
   *
   * Signature of a callback function that is called by Orthanc acting
   * as a HTTP server that supports chunked HTTP transfers. This
   * callback is invoked as soon as the full body of the HTTP request
   * is available. The plugin can then send its answer thanks to the
   * provided "output" object.
   * 
   * @see OrthancPluginRegisterChunkedRestCallback()
   * @param reader The user payload, as created by the OrthancPluginServerChunkedRequestReaderFactory() callback.
   * @param output The HTTP connection to the client application.
   * @return 0 if success, or the error code if failure.
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginServerChunkedRequestReaderExecute) (
    OrthancPluginServerChunkedRequestReader* reader,
    OrthancPluginRestOutput*                 output);
    

  /**
   * @brief Callback invoked to release the resources associated with an incoming HTTP chunked transfer.
   *
   * Signature of a callback function that is called by Orthanc acting
   * as a HTTP server that supports chunked HTTP transfers. This
   * callback is invoked to release all the resources allocated by the
   * given reader. Note that this function might be invoked even if
   * the entire body was not read, to deal with client error or
   * disconnection.
   * 
   * @see OrthancPluginRegisterChunkedRestCallback()
   * @param reader The user payload, as created by the OrthancPluginServerChunkedRequestReaderFactory() callback.
   **/
  typedef void (*OrthancPluginServerChunkedRequestReaderFinalize) (
    OrthancPluginServerChunkedRequestReader* reader);
  
  typedef struct
  {
    const char*                                      pathRegularExpression;
    OrthancPluginRestCallback                        getHandler;
    OrthancPluginServerChunkedRequestReaderFactory   postHandler;
    OrthancPluginRestCallback                        deleteHandler;
    OrthancPluginServerChunkedRequestReaderFactory   putHandler;
    OrthancPluginServerChunkedRequestReaderAddChunk  addChunk;
    OrthancPluginServerChunkedRequestReaderExecute   execute;
    OrthancPluginServerChunkedRequestReaderFinalize  finalize;
  } _OrthancPluginChunkedRestCallback;


  /**
   * @brief Register a REST callback to handle chunked HTTP transfers.
   *
   * This function registers a REST callback against a regular
   * expression for a URI. This function must be called during the
   * initialization of the plugin, i.e. inside the
   * OrthancPluginInitialize() public function.
   *
   * Contrarily to OrthancPluginRegisterRestCallback(), the callbacks
   * will NOT be invoked in mutual exclusion, so it is up to the
   * plugin to implement the required locking mechanisms.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param pathRegularExpression Regular expression for the URI. May contain groups. 
   * @param getHandler The callback function to handle REST calls using the GET HTTP method.
   * @param postHandler The callback function to handle REST calls using the POST HTTP method.
   * @param deleteHandler The callback function to handle REST calls using the DELETE HTTP method.
   * @param putHandler The callback function to handle REST calls using the PUT HTTP method.
   * @param addChunk The callback invoked when a new chunk is available for the request body of a POST or PUT call.
   * @param execute The callback invoked once the entire body of a POST or PUT call is read.
   * @param finalize The callback invoked to release the resources associated with a POST or PUT call.
   * @see OrthancPluginRegisterRestCallbackNoLock()
   *
   * @note
   * The regular expression is case sensitive and must follow the
   * [Perl syntax](https://www.boost.org/doc/libs/1_67_0/libs/regex/doc/html/boost_regex/syntax/perl_syntax.html).
   *
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginRegisterChunkedRestCallback(
    OrthancPluginContext*                            context,
    const char*                                      pathRegularExpression,
    OrthancPluginRestCallback                        getHandler,
    OrthancPluginServerChunkedRequestReaderFactory   postHandler,
    OrthancPluginRestCallback                        deleteHandler,
    OrthancPluginServerChunkedRequestReaderFactory   putHandler,
    OrthancPluginServerChunkedRequestReaderAddChunk  addChunk,
    OrthancPluginServerChunkedRequestReaderExecute   execute,
    OrthancPluginServerChunkedRequestReaderFinalize  finalize)
  {
    _OrthancPluginChunkedRestCallback params;
    params.pathRegularExpression = pathRegularExpression;
    params.getHandler = getHandler;
    params.postHandler = postHandler;
    params.deleteHandler = deleteHandler;
    params.putHandler = putHandler;
    params.addChunk = addChunk;
    params.execute = execute;
    params.finalize = finalize;

    context->InvokeService(context, _OrthancPluginService_RegisterChunkedRestCallback, &params);
  }





  typedef struct
  {
    char**       result;
    uint16_t     group;
    uint16_t     element;
    const char*  privateCreator;
  } _OrthancPluginGetTagName;

  /**
   * @brief Returns the symbolic name of a DICOM tag.
   *
   * This function makes a lookup to the dictionary of DICOM tags that
   * are known to Orthanc, and returns the symbolic name of a DICOM tag.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param group The group of the tag.
   * @param element The element of the tag.
   * @param privateCreator For private tags, the name of the private creator (can be NULL).
   * @return NULL in the case of an error, or a newly allocated string
   * containing the path. This string must be freed by
   * OrthancPluginFreeString().
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGetTagName(
    OrthancPluginContext*  context,
    uint16_t               group,
    uint16_t               element,
    const char*            privateCreator)
  {
    char* result;

    _OrthancPluginGetTagName params;
    params.result = &result;
    params.group = group;
    params.element = element;
    params.privateCreator = privateCreator;

    if (context->InvokeService(context, _OrthancPluginService_GetTagName, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }



  /**
   * @brief Callback executed by the storage commitment SCP.
   *
   * Signature of a factory function that creates an object to handle
   * one incoming storage commitment request.
   *
   * @remark The factory receives the list of the SOP class/instance
   * UIDs of interest to the remote storage commitment SCU. This gives
   * the factory the possibility to start some prefetch process
   * upfront in the background, before the handler object is actually
   * queried about the status of these DICOM instances.
   *
   * @param handler Output variable where the factory puts the handler object it created.
   * @param jobId ID of the Orthanc job that is responsible for handling 
   * the storage commitment request. This job will successively look for the
   * status of all the individual queried DICOM instances.
   * @param transactionUid UID of the storage commitment transaction
   * provided by the storage commitment SCU. It contains the value of the
   * (0008,1195) DICOM tag.
   * @param sopClassUids Array of the SOP class UIDs (0008,0016) that are queried by the SCU.
   * @param sopInstanceUids Array of the SOP instance UIDs (0008,0018) that are queried by the SCU.
   * @param countInstances Number of DICOM instances that are queried. This is the size
   * of the `sopClassUids` and `sopInstanceUids` arrays.
   * @param remoteAet The AET of the storage commitment SCU.
   * @param calledAet The AET used by the SCU to contact the storage commitment SCP (i.e. Orthanc).
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginStorageCommitmentFactory) (
    void**              handler /* out */,
    const char*         jobId,
    const char*         transactionUid,
    const char* const*  sopClassUids,
    const char* const*  sopInstanceUids,
    uint32_t            countInstances,
    const char*         remoteAet,
    const char*         calledAet);

  
  /**
   * @brief Callback to free one storage commitment SCP handler.
   * 
   * Signature of a callback function that releases the resources
   * allocated by the factory of the storage commitment SCP. The
   * handler is the return value of a previous call to the
   * OrthancPluginStorageCommitmentFactory() callback.
   *
   * @param handler The handler object to be destructed.
   * @ingroup DicomCallbacks
   **/
  typedef void (*OrthancPluginStorageCommitmentDestructor) (void* handler);


  /**
   * @brief Callback to get the status of one DICOM instance in the
   * storage commitment SCP.
   *
   * Signature of a callback function that is successively invoked for
   * each DICOM instance that is queried by the remote storage
   * commitment SCU.  The function must be tought of as a method of
   * the handler object that was created by a previous call to the
   * OrthancPluginStorageCommitmentFactory() callback. After each call
   * to this method, the progress of the associated Orthanc job is
   * updated.
   * 
   * @param target Output variable where to put the status for the queried instance.
   * @param handler The handler object associated with this storage commitment request.
   * @param sopClassUid The SOP class UID (0008,0016) of interest.
   * @param sopInstanceUid The SOP instance UID (0008,0018) of interest.
   * @ingroup DicomCallbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginStorageCommitmentLookup) (
    OrthancPluginStorageCommitmentFailureReason* target,
    void* handler,
    const char* sopClassUid,
    const char* sopInstanceUid);
    
    
  typedef struct
  {
    OrthancPluginStorageCommitmentFactory     factory;
    OrthancPluginStorageCommitmentDestructor  destructor;
    OrthancPluginStorageCommitmentLookup      lookup;
  } _OrthancPluginRegisterStorageCommitmentScpCallback;

  /**
   * @brief Register a callback to handle incoming requests to the storage commitment SCP.
   *
   * This function registers a callback to handle storage commitment SCP requests.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param factory Factory function that creates the handler object
   * for incoming storage commitment requests.
   * @param destructor Destructor function to destroy the handler object.
   * @param lookup Callback function to get the status of one DICOM instance.
   * @return 0 if success, other value if error.
   * @ingroup DicomCallbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterStorageCommitmentScpCallback(
    OrthancPluginContext*                     context,
    OrthancPluginStorageCommitmentFactory     factory,
    OrthancPluginStorageCommitmentDestructor  destructor,
    OrthancPluginStorageCommitmentLookup      lookup)
  {
    _OrthancPluginRegisterStorageCommitmentScpCallback params;
    params.factory = factory;
    params.destructor = destructor;
    params.lookup = lookup;
    return context->InvokeService(context, _OrthancPluginService_RegisterStorageCommitmentScpCallback, &params);
  }
  


  /**
   * @brief Callback to filter incoming DICOM instances received by Orthanc.
   *
   * Signature of a callback function that is triggered whenever
   * Orthanc receives a new DICOM instance (e.g. through REST API or
   * DICOM protocol), and that answers whether this DICOM instance
   * should be accepted or discarded by Orthanc.
   *
   * Note that the metadata information is not available
   * (i.e. GetInstanceMetadata() should not be used on "instance").
   *
   * @warning Your callback function will be called synchronously with
   * the core of Orthanc. This implies that deadlocks might emerge if
   * you call other core primitives of Orthanc in your callback (such
   * deadlocks are particularly visible in the presence of other plugins
   * or Lua scripts). It is thus strongly advised to avoid any call to
   * the REST API of Orthanc in the callback. If you have to call
   * other primitives of Orthanc, you should make these calls in a
   * separate thread, passing the pending events to be processed
   * through a message queue.
   * 
   * @param instance The received DICOM instance.
   * @return 0 to discard the instance, 1 to store the instance, -1 if error.
   * @ingroup Callbacks
   **/
  typedef int32_t (*OrthancPluginIncomingDicomInstanceFilter) (
    const OrthancPluginDicomInstance* instance);


  typedef struct
  {
    OrthancPluginIncomingDicomInstanceFilter callback;
  } _OrthancPluginIncomingDicomInstanceFilter;

  /**
   * @brief Register a callback to filter incoming DICOM instances.
   *
   * This function registers a custom callback to filter incoming
   * DICOM instances received by Orthanc (either through the REST API
   * or through the DICOM protocol).
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterIncomingDicomInstanceFilter(
    OrthancPluginContext*                     context,
    OrthancPluginIncomingDicomInstanceFilter  callback)
  {
    _OrthancPluginIncomingDicomInstanceFilter params;
    params.callback = callback;

    return context->InvokeService(context, _OrthancPluginService_RegisterIncomingDicomInstanceFilter, &params);
  }


  /**
   * @brief Callback to filter incoming DICOM instances received by 
   * Orthanc through C-STORE.
   *
   * Signature of a callback function that is triggered whenever
   * Orthanc receives a new DICOM instance using DICOM C-STORE, and
   * that answers whether this DICOM instance should be accepted or
   * discarded by Orthanc. If the instance is discarded, the callback
   * can specify the DIMSE error code answered by the Orthanc C-STORE
   * SCP.
   *
   * Note that the metadata information is not available
   * (i.e. GetInstanceMetadata() should not be used on "instance").
   *
   * @warning Your callback function will be called synchronously with
   * the core of Orthanc. This implies that deadlocks might emerge if
   * you call other core primitives of Orthanc in your callback (such
   * deadlocks are particularly visible in the presence of other plugins
   * or Lua scripts). It is thus strongly advised to avoid any call to
   * the REST API of Orthanc in the callback. If you have to call
   * other primitives of Orthanc, you should make these calls in a
   * separate thread, passing the pending events to be processed
   * through a message queue.
   *
   * @param dimseStatus If the DICOM instance is discarded, 
   * DIMSE status to be sent by the C-STORE SCP of Orthanc
   * @param instance The received DICOM instance.
   * @return 0 to discard the instance, 1 to store the instance, -1 if error.
   * @ingroup Callbacks
   **/
  typedef int32_t (*OrthancPluginIncomingCStoreInstanceFilter) (
    uint16_t* dimseStatus /* out */,
    const OrthancPluginDicomInstance* instance);


  typedef struct
  {
    OrthancPluginIncomingCStoreInstanceFilter callback;
  } _OrthancPluginIncomingCStoreInstanceFilter;

  /**
   * @brief Register a callback to filter incoming DICOM instances
   * received by Orthanc through C-STORE.
   *
   * This function registers a custom callback to filter incoming
   * DICOM instances received by Orthanc through the DICOM protocol.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterIncomingCStoreInstanceFilter(
    OrthancPluginContext*                      context,
    OrthancPluginIncomingCStoreInstanceFilter  callback)
  {
    _OrthancPluginIncomingCStoreInstanceFilter params;
    params.callback = callback;

    return context->InvokeService(context, _OrthancPluginService_RegisterIncomingCStoreInstanceFilter, &params);
  }

  /**
   * @brief Callback to keep/discard/modify a DICOM instance received
   * by Orthanc from any source (C-STORE or REST API)
   *
   * Signature of a callback function that is triggered whenever
   * Orthanc receives a new DICOM instance (through DICOM protocol or
   * REST API), and that specifies an action to be applied to this
   * newly received instance. The instance can be kept as it is, can
   * be modified by the plugin, or can be discarded.
   *
   * This callback is invoked immediately after reception, i.e. before
   * transcoding and before filtering
   * (cf. OrthancPluginRegisterIncomingDicomInstanceFilter()).
   *
   * @warning Your callback function will be called synchronously with
   * the core of Orthanc. This implies that deadlocks might emerge if
   * you call other core primitives of Orthanc in your callback (such
   * deadlocks are particularly visible in the presence of other plugins
   * or Lua scripts). It is thus strongly advised to avoid any call to
   * the REST API of Orthanc in the callback. If you have to call
   * other primitives of Orthanc, you should make these calls in a
   * separate thread, passing the pending events to be processed
   * through a message queue.
   *
   * @param modifiedDicomBuffer A buffer containing the modified DICOM (output).
   * This buffer must be allocated using OrthancPluginCreateMemoryBuffer64()
   * and will be freed by the Orthanc core.
   * @param receivedDicomBuffer A buffer containing the received DICOM (input).
   * @param receivedDicomBufferSize The size of the received DICOM (input).
   * @param origin The origin of the DICOM instance (input).
   * @return `OrthancPluginReceivedInstanceAction_KeepAsIs` to accept the instance as is,<br/>
   *         `OrthancPluginReceivedInstanceAction_Modify` to store the modified DICOM contained in `modifiedDicomBuffer`,<br/>
   *         `OrthancPluginReceivedInstanceAction_Discard` to tell Orthanc to discard the instance.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginReceivedInstanceAction (*OrthancPluginReceivedInstanceCallback) (
    OrthancPluginMemoryBuffer64* modifiedDicomBuffer,
    const void* receivedDicomBuffer,
    uint64_t receivedDicomBufferSize,
    OrthancPluginInstanceOrigin origin);


  typedef struct
  {
    OrthancPluginReceivedInstanceCallback callback;
  } _OrthancPluginReceivedInstanceCallback;

  /**
   * @brief Register a callback to keep/discard/modify a DICOM instance received
   * by Orthanc from any source (C-STORE or REST API)
   *
   * This function registers a custom callback to keep/discard/modify
   * incoming DICOM instances received by Orthanc from any source
   * (C-STORE or REST API).
   * 
   * @warning Contrarily to
   * OrthancPluginRegisterIncomingCStoreInstanceFilter() and
   * OrthancPluginRegisterIncomingDicomInstanceFilter() that can be
   * called by multiple plugins,
   * OrthancPluginRegisterReceivedInstanceCallback() can only be used
   * by one single plugin.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterReceivedInstanceCallback(
    OrthancPluginContext*                     context,
    OrthancPluginReceivedInstanceCallback     callback)
  {
    _OrthancPluginReceivedInstanceCallback params;
    params.callback = callback;

    return context->InvokeService(context, _OrthancPluginService_RegisterReceivedInstanceCallback, &params);
  }

  /**
   * @brief Get the transfer syntax of a DICOM file.
   *
   * This function returns a pointer to a newly created string that
   * contains the transfer syntax UID of the DICOM instance. The empty
   * string might be returned if this information is unknown.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @return The NULL value in case of error, or a string containing the
   * transfer syntax UID. This string must be freed by OrthancPluginFreeString().
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGetInstanceTransferSyntaxUid(
    OrthancPluginContext*              context,
    const OrthancPluginDicomInstance*  instance)
  {
    char* result;

    _OrthancPluginAccessDicomInstance params;
    memset(&params, 0, sizeof(params));
    params.resultStringToFree = &result;
    params.instance = instance;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceTransferSyntaxUid, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Check whether the DICOM file has pixel data.
   *
   * This function returns a Boolean value indicating whether the
   * DICOM instance contains the pixel data (7FE0,0010) tag.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @return "1" if the DICOM instance contains pixel data, or "0" if
   * the tag is missing, or "-1" in the case of an error.
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE int32_t OrthancPluginHasInstancePixelData(
    OrthancPluginContext*             context,
    const OrthancPluginDicomInstance* instance)
  {
    int64_t hasPixelData;

    _OrthancPluginAccessDicomInstance params;
    memset(&params, 0, sizeof(params));
    params.resultInt64 = &hasPixelData;
    params.instance = instance;

    if (context->InvokeService(context, _OrthancPluginService_HasInstancePixelData, &params) != OrthancPluginErrorCode_Success ||
        hasPixelData < 0 ||
        hasPixelData > 1)
    {
      /* Error */
      return -1;
    }
    else
    {
      return (hasPixelData != 0);
    }
  }






  typedef struct
  {
    OrthancPluginDicomInstance**  target;
    const void*                   buffer;
    uint32_t                      size;
    const char*                   transferSyntax;
  } _OrthancPluginCreateDicomInstance;

  /**
   * @brief Parse a DICOM instance.
   *
   * This function parses a memory buffer that contains a DICOM
   * file. The function returns a new pointer to a data structure that
   * is managed by the Orthanc core.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param buffer The memory buffer containing the DICOM instance.
   * @param size The size of the memory buffer.
   * @return The newly allocated DICOM instance. It must be freed with OrthancPluginFreeDicomInstance().
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginDicomInstance* OrthancPluginCreateDicomInstance(
    OrthancPluginContext*  context,
    const void*            buffer,
    uint32_t               size)
  {
    OrthancPluginDicomInstance* target = NULL;

    _OrthancPluginCreateDicomInstance params;
    params.target = &target;
    params.buffer = buffer;
    params.size = size;

    if (context->InvokeService(context, _OrthancPluginService_CreateDicomInstance, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }

  typedef struct
  {
    OrthancPluginDicomInstance*   dicom;
  } _OrthancPluginFreeDicomInstance;

  /**
   * @brief Free a DICOM instance.
   *
   * This function frees a DICOM instance that was parsed using
   * OrthancPluginCreateDicomInstance().
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param dicom The DICOM instance.
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE void  OrthancPluginFreeDicomInstance(
    OrthancPluginContext*        context, 
    OrthancPluginDicomInstance*  dicom)
  {
    _OrthancPluginFreeDicomInstance params;
    params.dicom = dicom;

    context->InvokeService(context, _OrthancPluginService_FreeDicomInstance, &params);
  }


  typedef struct
  {
    uint32_t*                             targetUint32;
    OrthancPluginMemoryBuffer*            targetBuffer;
    OrthancPluginImage**                  targetImage;
    char**                                targetStringToFree;
    const OrthancPluginDicomInstance*     instance;
    uint32_t                              frameIndex;
    OrthancPluginDicomToJsonFormat        format;
    OrthancPluginDicomToJsonFlags         flags;
    uint32_t                              maxStringLength;
    OrthancPluginDicomWebBinaryCallback2  dicomWebCallback;
    void*                                 dicomWebPayload;
  } _OrthancPluginAccessDicomInstance2;

  /**
   * @brief Get the number of frames in a DICOM instance.
   *
   * This function returns the number of frames that are part of a
   * DICOM image managed by the Orthanc core.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @return The number of frames (will be zero in the case of an error).
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE uint32_t OrthancPluginGetInstanceFramesCount(
    OrthancPluginContext*             context,
    const OrthancPluginDicomInstance* instance)
  {
    uint32_t count;

    _OrthancPluginAccessDicomInstance2 params;
    memset(&params, 0, sizeof(params));
    params.targetUint32 = &count;
    params.instance = instance;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceFramesCount, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return 0;
    }
    else
    {
      return count;
    }
  }


  /**
   * @brief Get the raw content of a frame in a DICOM instance.
   *
   * This function returns a memory buffer containing the raw content
   * of a frame in a DICOM instance that is managed by the Orthanc
   * core. This is notably useful for compressed transfer syntaxes, as
   * it gives access to the embedded files (such as JPEG, JPEG-LS or
   * JPEG2k). The Orthanc core transparently reassembles the fragments
   * to extract the raw frame.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param instance The instance of interest.
   * @param frameIndex The index of the frame of interest.
   * @return 0 if success, or the error code if failure.
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginGetInstanceRawFrame(
    OrthancPluginContext*             context,
    OrthancPluginMemoryBuffer*        target,
    const OrthancPluginDicomInstance* instance,
    uint32_t                          frameIndex)
  {
    _OrthancPluginAccessDicomInstance2 params;
    memset(&params, 0, sizeof(params));
    params.targetBuffer = target;
    params.instance = instance;
    params.frameIndex = frameIndex;

    return context->InvokeService(context, _OrthancPluginService_GetInstanceRawFrame, &params);
  }


  /**
   * @brief Decode one frame from a DICOM instance.
   *
   * This function decodes one frame of a DICOM image that is managed
   * by the Orthanc core.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The instance of interest.
   * @param frameIndex The index of the frame of interest.
   * @return The uncompressed image. It must be freed with OrthancPluginFreeImage().
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginImage* OrthancPluginGetInstanceDecodedFrame(
    OrthancPluginContext*             context,
    const OrthancPluginDicomInstance* instance,
    uint32_t                          frameIndex)
  {
    OrthancPluginImage* target = NULL;

    _OrthancPluginAccessDicomInstance2 params;
    memset(&params, 0, sizeof(params));
    params.targetImage = &target;
    params.instance = instance;
    params.frameIndex = frameIndex;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceDecodedFrame, &params) != OrthancPluginErrorCode_Success)
    {
      return NULL;
    }
    else
    {
      return target;
    }
  }

  
  /**
   * @brief Parse and transcode a DICOM instance.
   *
   * This function parses a memory buffer that contains a DICOM file,
   * then transcodes it to the given transfer syntax. The function
   * returns a new pointer to a data structure that is managed by the
   * Orthanc core.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param buffer The memory buffer containing the DICOM instance.
   * @param size The size of the memory buffer.
   * @param transferSyntax The transfer syntax UID for the transcoding.
   * @return The newly allocated DICOM instance. It must be freed with OrthancPluginFreeDicomInstance().
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginDicomInstance* OrthancPluginTranscodeDicomInstance(
    OrthancPluginContext*  context,
    const void*            buffer,
    uint32_t               size,
    const char*            transferSyntax)
  {
    OrthancPluginDicomInstance* target = NULL;

    _OrthancPluginCreateDicomInstance params;
    params.target = &target;
    params.buffer = buffer;
    params.size = size;
    params.transferSyntax = transferSyntax;

    if (context->InvokeService(context, _OrthancPluginService_TranscodeDicomInstance, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }

  /**
   * @brief Writes a DICOM instance to a memory buffer.
   *
   * This function returns a memory buffer containing the
   * serialization of a DICOM instance that is managed by the Orthanc
   * core.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param instance The instance of interest.
   * @return 0 if success, or the error code if failure.
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginSerializeDicomInstance(
    OrthancPluginContext*             context,
    OrthancPluginMemoryBuffer*        target,
    const OrthancPluginDicomInstance* instance)
  {
    _OrthancPluginAccessDicomInstance2 params;
    memset(&params, 0, sizeof(params));
    params.targetBuffer = target;
    params.instance = instance;

    return context->InvokeService(context, _OrthancPluginService_SerializeDicomInstance, &params);
  }
  

  /**
   * @brief Format a DICOM memory buffer as a JSON string.
   *
   * This function takes as DICOM instance managed by the Orthanc
   * core, and outputs a JSON string representing the tags of this
   * DICOM file.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The DICOM instance of interest.
   * @param format The output format.
   * @param flags Flags governing the output.
   * @param maxStringLength The maximum length of a field. Too long fields will
   * be output as "null". The 0 value means no maximum length.
   * @return The NULL value if the case of an error, or the JSON
   * string. This string must be freed by OrthancPluginFreeString().
   * @ingroup DicomInstance
   * @see OrthancPluginDicomBufferToJson
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGetInstanceAdvancedJson(
    OrthancPluginContext*              context,
    const OrthancPluginDicomInstance*  instance,
    OrthancPluginDicomToJsonFormat     format,
    OrthancPluginDicomToJsonFlags      flags, 
    uint32_t                           maxStringLength)
  {
    char* result = NULL;

    _OrthancPluginAccessDicomInstance2 params;
    memset(&params, 0, sizeof(params));
    params.targetStringToFree = &result;
    params.instance = instance;
    params.format = format;
    params.flags = flags;
    params.maxStringLength = maxStringLength;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceAdvancedJson, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


  /**
   * @brief Convert a DICOM instance to DICOMweb JSON.
   *
   * This function converts a DICOM instance that is managed by the
   * Orthanc core, into its DICOMweb JSON representation.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The DICOM instance of interest.
   * @param callback Callback to set the value of the binary tags.
   * @param payload User payload.
   * @return The NULL value in case of error, or the JSON document. This string must
   * be freed by OrthancPluginFreeString().
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGetInstanceDicomWebJson(
    OrthancPluginContext*                 context,
    const OrthancPluginDicomInstance*     instance,
    OrthancPluginDicomWebBinaryCallback2  callback,
    void*                                 payload)
  {
    char* target = NULL;
    
    _OrthancPluginAccessDicomInstance2 params;
    params.targetStringToFree = &target;
    params.instance = instance;
    params.dicomWebCallback = callback;
    params.dicomWebPayload = payload;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceDicomWebJson, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }
  

  /**
   * @brief Convert a DICOM instance to DICOMweb XML.
   *
   * This function converts a DICOM instance that is managed by the
   * Orthanc core, into its DICOMweb XML representation.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param instance The DICOM instance of interest.
   * @param callback Callback to set the value of the binary tags.
   * @param payload User payload.
   * @return The NULL value in case of error, or the XML document. This string must
   * be freed by OrthancPluginFreeString().
   * @ingroup DicomInstance
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGetInstanceDicomWebXml(
    OrthancPluginContext*                 context,
    const OrthancPluginDicomInstance*     instance,
    OrthancPluginDicomWebBinaryCallback2  callback,
    void*                                 payload)
  {
    char* target = NULL;
    
    _OrthancPluginAccessDicomInstance2 params;
    params.targetStringToFree = &target;
    params.instance = instance;
    params.dicomWebCallback = callback;
    params.dicomWebPayload = payload;

    if (context->InvokeService(context, _OrthancPluginService_GetInstanceDicomWebXml, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return target;
    }
  }



  /**
   * @brief Signature of a callback function to transcode a DICOM instance.
   * @param transcoded Target memory buffer. It must be allocated by the
   * plugin using OrthancPluginCreateMemoryBuffer().
   * @param buffer Memory buffer containing the source DICOM instance.
   * @param size Size of the source memory buffer.
   * @param allowedSyntaxes A C array of possible transfer syntaxes UIDs for the
   * result of the transcoding. The plugin must choose by itself the 
   * transfer syntax that will be used for the resulting DICOM image.
   * @param countSyntaxes The number of transfer syntaxes that are contained
   * in the "allowedSyntaxes" array.
   * @param allowNewSopInstanceUid Whether the transcoding plugin can select
   * a transfer syntax that will change the SOP instance UID (or, in other 
   * terms, whether the plugin can transcode using lossy compression).
   * @return 0 if success (i.e. image successfully transcoded and stored into
   * "transcoded"), or the error code if failure.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginTranscoderCallback) (
    OrthancPluginMemoryBuffer* transcoded /* out */,
    const void*                buffer,
    uint64_t                   size,
    const char* const*         allowedSyntaxes,
    uint32_t                   countSyntaxes,
    uint8_t                    allowNewSopInstanceUid);


  typedef struct
  {
    OrthancPluginTranscoderCallback callback;
  } _OrthancPluginTranscoderCallback;

  /**
   * @brief Register a callback to handle the transcoding of DICOM images.
   *
   * This function registers a custom callback to transcode DICOM
   * images, extending the built-in transcoder of Orthanc that uses
   * DCMTK. The exact behavior is affected by the configuration option
   * "BuiltinDecoderTranscoderOrder" of Orthanc.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param callback The callback.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterTranscoderCallback(
    OrthancPluginContext*            context,
    OrthancPluginTranscoderCallback  callback)
  {
    _OrthancPluginTranscoderCallback params;
    params.callback = callback;

    return context->InvokeService(context, _OrthancPluginService_RegisterTranscoderCallback, &params);
  }
  


  typedef struct
  {
    OrthancPluginMemoryBuffer*  target;
    uint32_t                    size;
  } _OrthancPluginCreateMemoryBuffer;

  /**
   * @brief Create a 32-bit memory buffer.
   *
   * This function creates a memory buffer that is managed by the
   * Orthanc core. The main use case of this function is for plugins
   * that act as DICOM transcoders.
   * 
   * Your plugin should never call "free()" on the resulting memory
   * buffer, as the C library that is used by the plugin is in general
   * not the same as the one used by the Orthanc core.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param size Size of the memory buffer to be created.
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginCreateMemoryBuffer(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  target,
    uint32_t                    size)
  {
    _OrthancPluginCreateMemoryBuffer params;
    params.target = target;
    params.size = size;

    return context->InvokeService(context, _OrthancPluginService_CreateMemoryBuffer, &params);
  }
  

  /**
   * @brief Generate a token to grant full access to the REST API of Orthanc.
   *
   * This function generates a token that can be set in the HTTP
   * header "Authorization" so as to grant full access to the REST API
   * of Orthanc using an external HTTP client. Using this function
   * avoids the need of adding a separate user in the
   * "RegisteredUsers" configuration of Orthanc, which eases
   * deployments.
   *
   * This feature is notably useful in multiprocess scenarios, where a
   * subprocess created by a plugin has no access to the
   * "OrthancPluginContext", and thus cannot call
   * "OrthancPluginRestApi[Get|Post|Put|Delete]()".
   *
   * This situation is frequently encountered in Python plugins, where
   * the "multiprocessing" package can be used to bypass the Global
   * Interpreter Lock (GIL) and thus to improve performance and
   * concurrency.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return The authorization token, or NULL value in the case of an error.
   * This string must be freed by OrthancPluginFreeString().
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE char* OrthancPluginGenerateRestApiAuthorizationToken(
    OrthancPluginContext*  context)
  {
    char* result;

    _OrthancPluginRetrieveDynamicString params;
    params.result = &result;
    params.argument = NULL;

    if (context->InvokeService(context, _OrthancPluginService_GenerateRestApiAuthorizationToken,
                               &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }



  typedef struct
  {
    OrthancPluginMemoryBuffer64*  target;
    uint64_t                      size;
  } _OrthancPluginCreateMemoryBuffer64;

  /**
   * @brief Create a 64-bit memory buffer.
   *
   * This function creates a 64-bit memory buffer that is managed by
   * the Orthanc core. The main use case of this function is for
   * plugins that read files from the storage area.
   * 
   * Your plugin should never call "free()" on the resulting memory
   * buffer, as the C library that is used by the plugin is in general
   * not the same as the one used by the Orthanc core.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param size Size of the memory buffer to be created.
   * @return 0 if success, or the error code if failure.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginCreateMemoryBuffer64(
    OrthancPluginContext*         context,
    OrthancPluginMemoryBuffer64*  target,
    uint64_t                      size)
  {
    _OrthancPluginCreateMemoryBuffer64 params;
    params.target = target;
    params.size = size;

    return context->InvokeService(context, _OrthancPluginService_CreateMemoryBuffer64, &params);
  }
  

  typedef struct
  {
    OrthancPluginStorageCreate    create;
    OrthancPluginStorageReadWhole readWhole;
    OrthancPluginStorageReadRange readRange;
    OrthancPluginStorageRemove    remove;
  } _OrthancPluginRegisterStorageArea2;

  /**
   * @brief Register a custom storage area, with support for range request.
   *
   * This function registers a custom storage area, to replace the
   * built-in way Orthanc stores its files on the filesystem. This
   * function must be called during the initialization of the plugin,
   * i.e. inside the OrthancPluginInitialize() public function.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param create The callback function to store a file on the custom storage area.
   * @param readWhole The callback function to read a whole file from the custom storage area.
   * @param readRange The callback function to read some range of a file from the custom storage area.
   * If this feature is not supported by the plugin, this value can be set to NULL.
   * @param remove The callback function to remove a file from the custom storage area.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE void OrthancPluginRegisterStorageArea2(
    OrthancPluginContext*          context,
    OrthancPluginStorageCreate     create,
    OrthancPluginStorageReadWhole  readWhole,
    OrthancPluginStorageReadRange  readRange,
    OrthancPluginStorageRemove     remove)
  {
    _OrthancPluginRegisterStorageArea2 params;
    params.create = create;
    params.readWhole = readWhole;
    params.readRange = readRange;
    params.remove = remove;
    context->InvokeService(context, _OrthancPluginService_RegisterStorageArea2, &params);
  }



  typedef struct
  {
    _OrthancPluginCreateDicom  createDicom;
    const char*                privateCreator;
  } _OrthancPluginCreateDicom2;

  /**
   * @brief Create a DICOM instance from a JSON string and an image, with a private creator.
   *
   * This function takes as input a string containing a JSON file
   * describing the content of a DICOM instance. As an output, it
   * writes the corresponding DICOM instance to a newly allocated
   * memory buffer. Additionally, an image to be encoded within the
   * DICOM instance can also be provided.
   *
   * Contrarily to the function OrthancPluginCreateDicom(), this
   * function can be explicitly provided with a private creator.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param target The target memory buffer. It must be freed with OrthancPluginFreeMemoryBuffer().
   * @param json The input JSON file.
   * @param pixelData The image. Can be NULL, if the pixel data is encoded inside the JSON with the data URI scheme.
   * @param flags Flags governing the output.
   * @param privateCreator The private creator to be used for the private DICOM tags.
   * Check out the global configuration option "Dictionary" of Orthanc.
   * @return 0 if success, other value if error.
   * @ingroup Toolbox
   * @see OrthancPluginCreateDicom
   * @see OrthancPluginDicomBufferToJson
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginCreateDicom2(
    OrthancPluginContext*          context,
    OrthancPluginMemoryBuffer*     target,
    const char*                    json,
    const OrthancPluginImage*      pixelData,
    OrthancPluginCreateDicomFlags  flags,
    const char*                    privateCreator)
  {
    _OrthancPluginCreateDicom2 params;
    params.createDicom.target = target;
    params.createDicom.json = json;
    params.createDicom.pixelData = pixelData;
    params.createDicom.flags = flags;
    params.privateCreator = privateCreator;

    return context->InvokeService(context, _OrthancPluginService_CreateDicom2, &params);
  }






  typedef struct
  {
    OrthancPluginMemoryBuffer*  answerBody;
    OrthancPluginMemoryBuffer*  answerHeaders;
    uint16_t*                   httpStatus;
    OrthancPluginHttpMethod     method;
    const char*                 uri;
    uint32_t                    headersCount;
    const char* const*          headersKeys;
    const char* const*          headersValues;
    const void*                 body;
    uint32_t                    bodySize;
    uint8_t                     afterPlugins;
  } _OrthancPluginCallRestApi;

  /**
   * @brief Call the REST API of Orthanc with full flexibility.
   * 
   * Make a call to the given URI in the REST API of Orthanc. The
   * result to the query is stored into a newly allocated memory
   * buffer. This function is always granted full access to the REST
   * API (no credentials, nor security token is needed).
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param answerBody The target memory buffer (out argument).
   *        It must be freed with OrthancPluginFreeMemoryBuffer().
   *        The value of this argument is ignored if the HTTP method is DELETE.
   * @param answerHeaders The target memory buffer for the HTTP headers in the answer (out argument). 
   *        The answer headers are formatted as a JSON object (associative array).
   *        The buffer must be freed with OrthancPluginFreeMemoryBuffer().
   *        This argument can be set to NULL if the plugin has no interest in the answer HTTP headers.
   * @param httpStatus The HTTP status after the execution of the request (out argument).
   * @param method HTTP method to be used.
   * @param uri The URI of interest.
   * @param headersCount The number of HTTP headers.
   * @param headersKeys Array containing the keys of the HTTP headers (can be <tt>NULL</tt> if no header).
   * @param headersValues Array containing the values of the HTTP headers (can be <tt>NULL</tt> if no header).
   * @param body The HTTP body for a POST or PUT request.
   * @param bodySize The size of the body.
   * @param afterPlugins If 0, the built-in API of Orthanc is used.
   * If 1, the API is tainted by the plugins.
   * @return 0 if success, or the error code if failure.
   * @see OrthancPluginRestApiGet2, OrthancPluginRestApiPost, OrthancPluginRestApiPut, OrthancPluginRestApiDelete
   * @ingroup Orthanc
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode  OrthancPluginCallRestApi(
    OrthancPluginContext*       context,
    OrthancPluginMemoryBuffer*  answerBody,
    OrthancPluginMemoryBuffer*  answerHeaders,
    uint16_t*                   httpStatus,
    OrthancPluginHttpMethod     method,
    const char*                 uri,
    uint32_t                    headersCount,
    const char* const*          headersKeys,
    const char* const*          headersValues,
    const void*                 body,
    uint32_t                    bodySize,
    uint8_t                     afterPlugins)
  {
    _OrthancPluginCallRestApi params;
    memset(&params, 0, sizeof(params));

    params.answerBody = answerBody;
    params.answerHeaders = answerHeaders;
    params.httpStatus = httpStatus;
    params.method = method;
    params.uri = uri;
    params.headersCount = headersCount;
    params.headersKeys = headersKeys;
    params.headersValues = headersValues;
    params.body = body;
    params.bodySize = bodySize;
    params.afterPlugins = afterPlugins;

    return context->InvokeService(context, _OrthancPluginService_CallRestApi, &params);
  }



  /**
   * @brief Opaque structure that represents a WebDAV collection.
   * @ingroup Callbacks
   **/
  typedef struct _OrthancPluginWebDavCollection_t OrthancPluginWebDavCollection;


  /**
   * @brief Declare a file while returning the content of a folder.
   *
   * This function declares a file while returning the content of a
   * WebDAV folder.
   *
   * @param collection Context of the collection.
   * @param name Base name of the file.
   * @param dateTime The date and time of creation of the file.
   * Check out the documentation of OrthancPluginWebDavRetrieveFile() for more information.
   * @param size Size of the file.
   * @param mimeType The MIME type of the file. If empty or set to `NULL`,
   * Orthanc will do a best guess depending on the file extension.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginWebDavAddFile) (
    OrthancPluginWebDavCollection*  collection,
    const char*                     name,
    uint64_t                        size,
    const char*                     mimeType,
    const char*                     dateTime);

  
  /**
   * @brief Declare a subfolder while returning the content of a folder.
   *
   * This function declares a subfolder while returning the content of a
   * WebDAV folder.
   *
   * @param collection Context of the collection.
   * @param name Base name of the subfolder.
   * @param dateTime The date and time of creation of the subfolder.
   * Check out the documentation of OrthancPluginWebDavRetrieveFile() for more information.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginWebDavAddFolder) (
    OrthancPluginWebDavCollection*  collection,
    const char*                     name,
    const char*                     dateTime);


  /**
   * @brief Retrieve the content of a file.
   *
   * This function is used to forward the content of a file from a
   * WebDAV collection, to the core of Orthanc.
   *
   * @param collection Context of the collection.
   * @param data Content of the file.
   * @param size Size of the file.
   * @param mimeType The MIME type of the file. If empty or set to `NULL`,
   * Orthanc will do a best guess depending on the file extension.
   * @param dateTime The date and time of creation of the file.
   * It must be formatted as an ISO string of form
   * `YYYYMMDDTHHMMSS,fffffffff` where T is the date-time
   * separator. It must be expressed in UTC (it is the responsibility
   * of the plugin to do the possible timezone
   * conversions). Internally, this string will be parsed using
   * `boost::posix_time::from_iso_string()`.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginWebDavRetrieveFile) (
    OrthancPluginWebDavCollection*  collection,
    const void*                     data,
    uint64_t                        size,
    const char*                     mimeType,
    const char*                     dateTime);

  
  /**
   * @brief Callback for testing the existence of a folder.
   *
   * Signature of a callback function that tests whether the given
   * path in the WebDAV collection exists and corresponds to a folder.
   *
   * @param isExisting Pointer to a Boolean that must be set to `1` if the folder exists, or `0` otherwise.
   * @param pathSize Number of levels in the path.
   * @param pathItems Items making the path.
   * @param payload The user payload.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginWebDavIsExistingFolderCallback) (
    uint8_t*                        isExisting, /* out */
    uint32_t                        pathSize,
    const char* const*              pathItems,
    void*                           payload);

  
  /**
   * @brief Callback for listing the content of a folder.
   *
   * Signature of a callback function that lists the content of a
   * folder in the WebDAV collection. The callback must call the
   * provided `addFile()` and `addFolder()` functions to emit the
   * content of the folder.
   *
   * @param isExisting Pointer to a Boolean that must be set to `1` if the folder exists, or `0` otherwise.
   * @param collection Context to be provided to `addFile()` and `addFolder()` functions.
   * @param addFile Function to add a file to the list.
   * @param addFolder Function to add a folder to the list.
   * @param pathSize Number of levels in the path.
   * @param pathItems Items making the path.
   * @param payload The user payload.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginWebDavListFolderCallback) (
    uint8_t*                        isExisting, /* out */
    OrthancPluginWebDavCollection*  collection,
    OrthancPluginWebDavAddFile      addFile,
    OrthancPluginWebDavAddFolder    addFolder,
    uint32_t                        pathSize,
    const char* const*              pathItems,
    void*                           payload);

  
  /**
   * @brief Callback for retrieving the content of a file.
   *
   * Signature of a callback function that retrieves the content of a
   * file in the WebDAV collection. The callback must call the
   * provided `retrieveFile()` function to emit the actual content of
   * the file.
   *
   * @param collection Context to be provided to `retrieveFile()` function.
   * @param retrieveFile Function to return the content of the file.
   * @param pathSize Number of levels in the path.
   * @param pathItems Items making the path.
   * @param payload The user payload.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginWebDavRetrieveFileCallback) (
    OrthancPluginWebDavCollection*  collection,
    OrthancPluginWebDavRetrieveFile retrieveFile,
    uint32_t                        pathSize,
    const char* const*              pathItems,
    void*                           payload);

  
  /**
   * @brief Callback to store a file.
   *
   * Signature of a callback function that stores a file into the
   * WebDAV collection.
   *
   * @param isReadOnly Pointer to a Boolean that must be set to `1` if the collection is read-only, or `0` otherwise.
   * @param pathSize Number of levels in the path.
   * @param pathItems Items making the path.
   * @param data Content of the file to be stored.
   * @param size Size of the file to be stored.
   * @param payload The user payload.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginWebDavStoreFileCallback) (
    uint8_t*                        isReadOnly, /* out */
    uint32_t                        pathSize,
    const char* const*              pathItems,
    const void*                     data,
    uint64_t                        size,
    void*                           payload);

  
  /**
   * @brief Callback to create a folder.
   *
   * Signature of a callback function that creates a folder in the
   * WebDAV collection.
   *
   * @param isReadOnly Pointer to a Boolean that must be set to `1` if the collection is read-only, or `0` otherwise.
   * @param pathSize Number of levels in the path.
   * @param pathItems Items making the path.
   * @param payload The user payload.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginWebDavCreateFolderCallback) (
    uint8_t*                        isReadOnly, /* out */
    uint32_t                        pathSize,
    const char* const*              pathItems,
    void*                           payload);

  
  /**
   * @brief Callback to remove a file or a folder.
   *
   * Signature of a callback function that removes a file or a folder
   * from the WebDAV collection.
   *
   * @param isReadOnly Pointer to a Boolean that must be set to `1` if the collection is read-only, or `0` otherwise.
   * @param pathSize Number of levels in the path.
   * @param pathItems Items making the path.
   * @param payload The user payload.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  typedef OrthancPluginErrorCode (*OrthancPluginWebDavDeleteItemCallback) (
    uint8_t*                        isReadOnly, /* out */
    uint32_t                        pathSize,
    const char* const*              pathItems,
    void*                           payload);

  
  typedef struct
  {
    const char*                                  uri;
    OrthancPluginWebDavIsExistingFolderCallback  isExistingFolder;
    OrthancPluginWebDavListFolderCallback        listFolder;
    OrthancPluginWebDavRetrieveFileCallback      retrieveFile;
    OrthancPluginWebDavStoreFileCallback         storeFile;
    OrthancPluginWebDavCreateFolderCallback      createFolder;
    OrthancPluginWebDavDeleteItemCallback        deleteItem;
    void*                                        payload;
  } _OrthancPluginRegisterWebDavCollection;

  /**
   * @brief Register a WebDAV virtual filesystem.
   *
   * This function maps a WebDAV collection onto the given URI in the
   * REST API of Orthanc. This function must be called during the
   * initialization of the plugin, i.e. inside the
   * OrthancPluginInitialize() public function.
   * 
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @param uri URI where to map the WebDAV collection (must start with a `/` character).
   * @param isExistingFolder Callback method to test for the existence of a folder.
   * @param listFolder Callback method to list the content of a folder.
   * @param retrieveFile Callback method to retrieve the content of a file.
   * @param storeFile Callback method to store a file into the WebDAV collection.
   * @param createFolder Callback method to create a folder.
   * @param deleteItem Callback method to delete a file or a folder.
   * @param payload The user payload.
   * @return 0 if success, other value if error.
   * @ingroup Callbacks
   **/
  ORTHANC_PLUGIN_INLINE OrthancPluginErrorCode OrthancPluginRegisterWebDavCollection(
    OrthancPluginContext*                        context,
    const char*                                  uri,
    OrthancPluginWebDavIsExistingFolderCallback  isExistingFolder,
    OrthancPluginWebDavListFolderCallback        listFolder,
    OrthancPluginWebDavRetrieveFileCallback      retrieveFile,
    OrthancPluginWebDavStoreFileCallback         storeFile,
    OrthancPluginWebDavCreateFolderCallback      createFolder,
    OrthancPluginWebDavDeleteItemCallback        deleteItem,
    void*                                        payload)
  {
    _OrthancPluginRegisterWebDavCollection params;
    params.uri = uri;
    params.isExistingFolder = isExistingFolder;
    params.listFolder = listFolder;
    params.retrieveFile = retrieveFile;
    params.storeFile = storeFile;
    params.createFolder = createFolder;
    params.deleteItem = deleteItem;
    params.payload = payload;

    return context->InvokeService(context, _OrthancPluginService_RegisterWebDavCollection, &params);
  }


  /**
   * @brief Gets the DatabaseServerIdentifier.
   *
   * @param context The Orthanc plugin context, as received by OrthancPluginInitialize().
   * @return the database server identifier.  This is a statically-allocated
   * string, do not free it.
   * @ingroup Toolbox
   **/
  ORTHANC_PLUGIN_INLINE const char* OrthancPluginGetDatabaseServerIdentifier(
    OrthancPluginContext*  context)
  {
    const char* result;

    _OrthancPluginRetrieveStaticString params;
    params.result = &result;
    params.argument = NULL;

    if (context->InvokeService(context, _OrthancPluginService_GetDatabaseServerIdentifier, &params) != OrthancPluginErrorCode_Success)
    {
      /* Error */
      return NULL;
    }
    else
    {
      return result;
    }
  }


#ifdef  __cplusplus
}
#endif


/** @} */

