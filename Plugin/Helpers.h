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

#pragma once

#include "../Resources/Orthanc/Plugins/OrthancPluginCppWrapper.h"
#include <HttpClient.h>

namespace OrthancPlugins
{
  Orthanc::HttpMethod Convert(OrthancPluginHttpMethod method);

  void ForwardToWebService(OrthancPluginRestOutput* output,
                          const OrthancPluginHttpRequest* request,
                          const Orthanc::WebServiceParameters& webServiceParameters,
                          const std::string& webServiceUrl);

}