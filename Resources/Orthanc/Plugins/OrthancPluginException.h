/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 * Copyright (C) 2017-2023 Osimis S.A., Belgium
 * Copyright (C) 2024-2025 Orthanc Team SRL, Belgium
 * Copyright (C) 2021-2025 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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

#if !defined(HAS_ORTHANC_EXCEPTION)
#  error The macro HAS_ORTHANC_EXCEPTION must be defined
#endif


#if HAS_ORTHANC_EXCEPTION == 1
#  include <OrthancException.h>
#  define ORTHANC_PLUGINS_ERROR_ENUMERATION     ::Orthanc::ErrorCode
#  define ORTHANC_PLUGINS_EXCEPTION_CLASS       ::Orthanc::OrthancException
#  define ORTHANC_PLUGINS_GET_ERROR_CODE(code)  ::Orthanc::ErrorCode_ ## code
#else
#  include <orthanc/OrthancCPlugin.h>
#  define ORTHANC_PLUGINS_ERROR_ENUMERATION     ::OrthancPluginErrorCode
#  define ORTHANC_PLUGINS_EXCEPTION_CLASS       ::OrthancPlugins::PluginException
#  define ORTHANC_PLUGINS_GET_ERROR_CODE(code)  ::OrthancPluginErrorCode_ ## code
#endif


#define ORTHANC_PLUGINS_THROW_PLUGIN_ERROR_CODE(code)                   \
  throw ORTHANC_PLUGINS_EXCEPTION_CLASS(static_cast<ORTHANC_PLUGINS_ERROR_ENUMERATION>(code));


#define ORTHANC_PLUGINS_THROW_EXCEPTION(code)                           \
  throw ORTHANC_PLUGINS_EXCEPTION_CLASS(ORTHANC_PLUGINS_GET_ERROR_CODE(code));
                                                  

#define ORTHANC_PLUGINS_CHECK_ERROR(code)                           \
  if (code != ORTHANC_PLUGINS_GET_ERROR_CODE(Success))              \
  {                                                                 \
    ORTHANC_PLUGINS_THROW_EXCEPTION(code);                          \
  }


namespace OrthancPlugins
{
#if HAS_ORTHANC_EXCEPTION == 0
  class PluginException
  {
  private:
    OrthancPluginErrorCode  code_;

  public:
    explicit PluginException(OrthancPluginErrorCode code) : code_(code)
    {
    }

    OrthancPluginErrorCode GetErrorCode() const
    {
      return code_;
    }

    const char* What(OrthancPluginContext* context) const
    {
      const char* description = OrthancPluginGetErrorDescription(context, code_);
      if (description)
      {
        return description;
      }
      else
      {
        return "No description available";
      }
    }
  };
#endif
}
