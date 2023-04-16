# Orthanc - A Lightweight, RESTful DICOM Store
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2022 Osimis S.A., Belgium
# Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.


cmake_minimum_required(VERSION 3.1)

if (APPLE)
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
  set(CMAKE_CXX_STANDARD 11)
endif(APPLE)

project(OrthancExplorer2)

set(PLUGIN_VERSION "mainline" CACHE STRING "plugin version 'mainline' or '1.2.3'")

if (PLUGIN_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
  set(ORTHANC_FRAMEWORK_DEFAULT_VERSION "1.11.2")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "web")
  set(ORTHANC_OE2_VERSION ${PLUGIN_VERSION})
else()
  set(ORTHANC_FRAMEWORK_DEFAULT_VERSION "mainline")
  set(ORTHANC_FRAMEWORK_DEFAULT_SOURCE "hg")
  set(ORTHANC_OE2_VERSION "mainline")
endif()


# Parameters of the build
set(STATIC_BUILD OFF CACHE BOOL "Static build of the third-party libraries (necessary for Windows)")
set(STANDALONE_BUILD ON CACHE BOOL "Standalone build (all the resources are embedded, necessary for releases)")
set(ALLOW_DOWNLOADS OFF CACHE BOOL "Allow CMake to download packages")
set(ORTHANC_FRAMEWORK_SOURCE "${ORTHANC_FRAMEWORK_DEFAULT_SOURCE}" CACHE STRING "Source of the Orthanc framework (can be \"system\", \"hg\", \"archive\", \"web\" or \"path\")")
set(ORTHANC_FRAMEWORK_VERSION "${ORTHANC_FRAMEWORK_DEFAULT_VERSION}" CACHE STRING "Version of the Orthanc framework")
set(ORTHANC_FRAMEWORK_ARCHIVE "" CACHE STRING "Path to the Orthanc archive, if ORTHANC_FRAMEWORK_SOURCE is \"archive\"")
set(ORTHANC_FRAMEWORK_ROOT "" CACHE STRING "Path to the Orthanc source directory, if ORTHANC_FRAMEWORK_SOURCE is \"path\"")
set(WEBAPP_DIST_PATH "${CMAKE_SOURCE_DIR}/WebApplication/dist" CACHE STRING "Path to the WebApplication/dist folder (the output of 'npm run build')")
set(WEBAPP_DIST_SOURCE "WEB" CACHE STRING "Source for the WebApplication/dist folder ('LOCAL' = assume it is build localy, 'WEB' = download it from web)")
set(WEBAPP_DIST_VERSION "${PLUGIN_VERSION}" CACHE STRING "Version of WebApplication/dist folder to download")
file(TO_CMAKE_PATH "${WEBAPP_DIST_PATH}" WEBAPP_DIST_PATH)

# Advanced parameters to fine-tune linking against system libraries
set(USE_SYSTEM_ORTHANC_SDK ON CACHE BOOL "Use the system version of the Orthanc plugin SDK")
set(ORTHANC_SDK_VERSION "1.11.2" CACHE STRING "Version of the Orthanc plugin SDK to use, if not using the system version (can be \"1.11.2\")")
set(ORTHANC_FRAMEWORK_STATIC OFF CACHE BOOL "If linking against the Orthanc framework system library, indicates whether this library was statically linked")
mark_as_advanced(ORTHANC_FRAMEWORK_STATIC)


# Download and setup the Orthanc framework
include(${CMAKE_SOURCE_DIR}/Resources/Orthanc/CMake/DownloadOrthancFramework.cmake)

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "system")
  if (ORTHANC_FRAMEWORK_USE_SHARED)
    include(FindBoost)
    find_package(Boost COMPONENTS regex thread)
    
    if (NOT Boost_FOUND)
      message(FATAL_ERROR "Unable to locate Boost on this system")
    endif()
    
    link_libraries(${Boost_LIBRARIES} jsoncpp)
  endif()

  link_libraries(${ORTHANC_FRAMEWORK_LIBRARIES})
  
  set(USE_SYSTEM_GOOGLE_TEST ON CACHE BOOL "Use the system version of Google Test")
  set(USE_GOOGLE_TEST_DEBIAN_PACKAGE OFF CACHE BOOL "Use the sources of Google Test shipped with libgtest-dev (Debian only)")
  mark_as_advanced(USE_GOOGLE_TEST_DEBIAN_PACKAGE)
  include(${CMAKE_SOURCE_DIR}/Resources/Orthanc/CMake/GoogleTestConfiguration.cmake)
  
else()
  include(${ORTHANC_FRAMEWORK_ROOT}/../Resources/CMake/OrthancFrameworkParameters.cmake)

  set(ENABLE_LOCALE OFF)         # Enable support for locales (notably in Boost)
  #set(ENABLE_GOOGLE_TEST ON)
  #set(ENABLE_WEB_CLIENT ON)

  # Those modules of the Orthanc framework are not needed
  set(ENABLE_MODULE_IMAGES OFF)
  set(ENABLE_MODULE_JOBS OFF)
  set(ENABLE_MODULE_DICOM OFF)

  include(${ORTHANC_FRAMEWORK_ROOT}/../Resources/CMake/OrthancFrameworkConfiguration.cmake)
  include_directories(${ORTHANC_FRAMEWORK_ROOT})
endif()


include(${CMAKE_SOURCE_DIR}/Resources/Orthanc/Plugins/OrthancPluginsExports.cmake)


if (STATIC_BUILD OR NOT USE_SYSTEM_ORTHANC_SDK)
  if (ORTHANC_SDK_VERSION STREQUAL "1.11.2")
    include_directories(${CMAKE_SOURCE_DIR}/Resources/Orthanc/Sdk-1.11.2)
  else()
    message(FATAL_ERROR "Unsupported version of the Orthanc plugin SDK: ${ORTHANC_SDK_VERSION}")
  endif()
else ()
  CHECK_INCLUDE_FILE_CXX(orthanc/OrthancCPlugin.h HAVE_ORTHANC_H)
  if (NOT HAVE_ORTHANC_H)
    message(FATAL_ERROR "Please install the headers of the Orthanc plugins SDK")
  endif()
endif()


if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "kFreeBSD" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  link_libraries(rt)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  SET(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lws2_32")

  execute_process(
    COMMAND 
    ${PYTHON_EXECUTABLE} ${ORTHANC_FRAMEWORK_ROOT}/../Resources/WindowsResources.py
    ${ORTHANC_OE2_VERSION} "Orthanc Explorer 2 plugin" OrthancExplorer2.dll
    "Advanced User Interface Plugin for Orthanc"
    ERROR_VARIABLE Failure
    OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/Version.rc
    )

  if (Failure)
    message(FATAL_ERROR "Error while computing the version information: ${Failure}")
  endif()

  list(APPEND AUTOGENERATED_SOURCES  ${CMAKE_CURRENT_BINARY_DIR}/Version.rc)
endif()


if (APPLE)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreFoundation")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework CoreFoundation")
endif()

if (${WEBAPP_DIST_SOURCE} STREQUAL "WEB")
  if (${WEBAPP_DIST_VERSION} STREQUAL "mainline")
    set(WEBAPP_DIST_VERSION "master")  # the branch is named master, not mainline
  endif()
  DownloadPackage(no-check "https://orthanc.osimis.io/public/oe2/web-app-dist/${WEBAPP_DIST_VERSION}.zip" ${WEBAPP_DIST_PATH})
endif()

if (STANDALONE_BUILD)
  add_definitions(-DORTHANC_STANDALONE=1)
  set(ADDITIONAL_RESOURCES
    DEFAULT_CONFIGURATION  ${CMAKE_SOURCE_DIR}/Plugin/DefaultConfiguration.json
    WEB_APPLICATION_ASSETS   ${WEBAPP_DIST_PATH}/assets
    WEB_APPLICATION_FAVICON   ${WEBAPP_DIST_PATH}/favicon.ico
    WEB_APPLICATION_INDEX   ${WEBAPP_DIST_PATH}/index.html
    WEB_APPLICATION_INDEX_LANDING   ${WEBAPP_DIST_PATH}/token-landing.html
    WEB_APPLICATION_INDEX_RETRIEVE_AND_VIEW   ${WEBAPP_DIST_PATH}/retrieve-and-view.html
    )
else()
  add_definitions(-DORTHANC_STANDALONE=0)
endif()

EmbedResources(
  --no-upcase-check
  ${ADDITIONAL_RESOURCES}
  ORTHANC_EXPLORER  ${CMAKE_SOURCE_DIR}/Plugin/OrthancExplorer.js
  )


# As the embedded resources are shared by both the "UnitTests" and the
# "OrthancExplorer2" targets, avoid race conditions in the code
# generation by adding a target between them
add_custom_target(
  AutogeneratedTarget
  DEPENDS 
  ${AUTOGENERATED_SOURCES}
  )


add_definitions(
  -DHAS_ORTHANC_EXCEPTION=1
  -DORTHANC_ENABLE_LOGGING_PLUGIN=1
  )

set(CORE_SOURCES
#   Plugin/Configuration.cpp

  ${CMAKE_SOURCE_DIR}/Resources/Orthanc/Plugins/OrthancPluginCppWrapper.cpp
  ${ORTHANC_CORE_SOURCES}
  )

add_library(OrthancExplorer2 SHARED ${CORE_SOURCES}
  ${CMAKE_SOURCE_DIR}/Plugin/Plugin.cpp
  ${AUTOGENERATED_SOURCES}
  )

add_dependencies(OrthancExplorer2 AutogeneratedTarget)

message("Setting the version of the library to ${ORTHANC_OE2_VERSION}")

add_definitions(-DORTHANC_OE2_VERSION="${ORTHANC_OE2_VERSION}")

set_target_properties(OrthancExplorer2 PROPERTIES 
  VERSION ${ORTHANC_OE2_VERSION} 
  SOVERSION ${ORTHANC_OE2_VERSION}
  )

install(
  TARGETS OrthancExplorer2
  RUNTIME DESTINATION lib    # Destination for Windows
  LIBRARY DESTINATION share/orthanc/plugins    # Destination for Linux
  )

# add_executable(UnitTests
#   ${AUTOGENERATED_SOURCES}
#   ${CORE_SOURCES}
#   ${GOOGLE_TEST_SOURCES}
# #   UnitTestsSources/UnitTestsMain.cpp
#   )

# add_dependencies(UnitTests AutogeneratedTarget)

# target_link_libraries(UnitTests
#   ${GOOGLE_TEST_LIBRARIES}
#   )
