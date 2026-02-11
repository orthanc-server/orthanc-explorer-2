/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2024 Osimis S.A., Belgium
 * Copyright (C) 2021-2026 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
 * Copyright (C) 2024-2026 Orthanc Team SRL, Belgium
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

#include "Helpers.h"

namespace OrthancPlugins
{
  Orthanc::HttpMethod Convert(OrthancPluginHttpMethod method)
  {
    switch (method)
    {
      case OrthancPluginHttpMethod_Get:
        return Orthanc::HttpMethod_Get;

      case OrthancPluginHttpMethod_Post:
        return Orthanc::HttpMethod_Post;

      case OrthancPluginHttpMethod_Put:
        return Orthanc::HttpMethod_Put;

      case OrthancPluginHttpMethod_Delete:
        return Orthanc::HttpMethod_Delete;

      default:
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
    }
  }

  void ForwardToWebService(OrthancPluginRestOutput* output,
                          const OrthancPluginHttpRequest* request,
                          const Orthanc::WebServiceParameters& webServiceParameters,
                          const std::string& webServiceUrl)
  {
    Orthanc::HttpClient client(webServiceParameters, webServiceUrl);
    client.SetMethod(Convert(request->method));
    
    for (uint32_t h = 0; h < request->headersCount; ++h)
    {
      if (strcmp(request->headersKeys[h], "content-type") == 0)
      {
        client.AddHeader(request->headersKeys[h], request->headersValues[h]);
      }
    }

    if (request->bodySize > 0)
    {
      client.AssignBody(request->body, request->bodySize);
    }

    HttpHeaders answerHeaders;
    std::string answerBody;

    std::string response;
    Orthanc::HttpClient::HttpHeaders responseHeaders;

    bool success = client.Apply(response, responseHeaders);
    const char* mimeType = NULL;

    if (response.size() > 0)
    {
      for (HttpHeaders::const_iterator it = responseHeaders.begin(); it != responseHeaders.end(); ++it)
      {
        if (it->first == "content-type")
        {
          mimeType = it->second.c_str();
        }
      }
    }

    if (success)
    {
      OrthancPlugins::AnswerString(response, mimeType, output);
    }
    else
    {
      if (response.size() > 0)
      {
        OrthancPlugins::AnswerHttpError(client.GetLastStatus(), output, response, mimeType);
      }
      else
      {
        OrthancPlugins::AnswerHttpError(client.GetLastStatus(), output);
      }
    }
    // {
    //   if ()

    //   OrthancPlugins::AnswerString(response, Orthanc::MIME_JSON_UTF8, output);
    //   return;
    // }
    // else
    // {
    //   LOG(ERROR) << "Failed to send email from the email web-service; check the web-service logs.";
    //   if (response.size() > 0)  
    //   {
    //     response
    //     OrthancPlugins::AnswerHttpError(httpClient.GetLastStatus(), output, )
    //     OrthancPlugins::AnswerString(response, Orthanc::MIME_JSON_UTF8, output);
    //   }
  
    //   OrthancPlugins::AnswerHttpError(httpClient.GetLastStatus(), output);
    // }


    // client.Execute(answerHeaders, answerBody);

    // if (answerBody.size() > 0)
    // {
    //   const char* mimeType = NULL;
    //   for (HttpHeaders::const_iterator it = answerHeaders.begin(); it != answerHeaders.end(); ++it)
    //   {
    //     if (it->first == "content-type")
    //     {
    //       mimeType = it->second.c_str();
    //     }
    //   }

    //   if (client.GetHttpStatus() == 200)
    //   {
    //     OrthancPluginAnswerBuffer(GetGlobalContext(), output, answerBody.c_str(), answerBody.size(), mimeType);
    //     // OrthancPlugins::AnswerString(answerBody, Orthanc::MIME_HTML, output);
    //   }
    //   else
    //   {
    //     OrthancPlugins::AnswerHttpError(client.GetHttpStatus(), output, answerBody, mimeType);
    //   }
    // }
    // else
    // {
    //   OrthancPluginSendHttpStatusCode(GetGlobalContext(), output, client.GetHttpStatus());
    // }      



    // std::string templateContent;
    // if (httpClient.Apply(templateContent))
    // {
    //   OrthancPlugins::AnswerString(templateContent, Orthanc::MIME_HTML, output);
    //   return;
    // }

    // LOG(ERROR) << "Failed to retrieve template '" << templateName << "' from the email web-service; check the web-service logs.";
    
    // OrthancPlugins::AnswerHttpError(httpClient.GetLastStatus(), output);


    // OrthancPlugins::HttpClient client;
    
    // if (request->method == OrthancPluginHttpMethod_Post)
    // {
    //   std::cout << "break";
    // }

    // client.SetUrl(webServiceUrl);

    // client.SetMethod(request->method);
    // for (uint32_t h = 0; h < request->headersCount; ++h)
    // {
    //   if (strcmp(request->headersKeys[h], "content-type") == 0)
    //   {
    //     client.AddHeader(request->headersKeys[h], request->headersValues[h]);
    //   }
    // }

    // if (!webServiceUserName.empty() && !webServicePassword.empty())
    // {
    //   client.SetCredentials(webServiceUserName, webServicePassword);
    // }

    // if (request->bodySize > 0)
    // {
    //   client.SetBody(std::string(reinterpret_cast<const char*>(request->body), request->bodySize));
    // }

    // HttpHeaders answerHeaders;
    // std::string answerBody;

    // client.Execute(answerHeaders, answerBody);

    // if (answerBody.size() > 0)
    // {
    //   const char* mimeType = NULL;
    //   for (HttpHeaders::const_iterator it = answerHeaders.begin(); it != answerHeaders.end(); ++it)
    //   {
    //     if (it->first == "content-type")
    //     {
    //       mimeType = it->second.c_str();
    //     }
    //   }

    //   if (client.GetHttpStatus() == 200)
    //   {
    //     OrthancPluginAnswerBuffer(GetGlobalContext(), output, answerBody.c_str(), answerBody.size(), mimeType);
    //     // OrthancPlugins::AnswerString(answerBody, Orthanc::MIME_HTML, output);
    //   }
    //   else
    //   {
    //     OrthancPlugins::AnswerHttpError(client.GetHttpStatus(), output, answerBody, mimeType);
    //   }
    // }
    // else
    // {
    //   OrthancPluginSendHttpStatusCode(GetGlobalContext(), output, client.GetHttpStatus());
    // }      

  }

}