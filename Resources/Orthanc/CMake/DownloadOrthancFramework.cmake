# Orthanc - A Lightweight, RESTful DICOM Store
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2023 Osimis S.A., Belgium
# Copyright (C) 2021-2023 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program. If not, see
# <http://www.gnu.org/licenses/>.



##
## Check whether the parent script sets the mandatory variables
##

if (NOT DEFINED ORTHANC_FRAMEWORK_SOURCE OR
    (NOT ORTHANC_FRAMEWORK_SOURCE STREQUAL "system" AND
     NOT ORTHANC_FRAMEWORK_SOURCE STREQUAL "hg" AND
     NOT ORTHANC_FRAMEWORK_SOURCE STREQUAL "web" AND
     NOT ORTHANC_FRAMEWORK_SOURCE STREQUAL "archive" AND
     NOT ORTHANC_FRAMEWORK_SOURCE STREQUAL "path"))
  message(FATAL_ERROR "The variable ORTHANC_FRAMEWORK_SOURCE must be set to \"system\", \"hg\", \"web\", \"archive\" or \"path\"")
endif()


##
## Detection of the requested version
##

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "hg" OR
    ORTHANC_FRAMEWORK_SOURCE STREQUAL "archive" OR
    ORTHANC_FRAMEWORK_SOURCE STREQUAL "web")
  if (NOT DEFINED ORTHANC_FRAMEWORK_VERSION)
    message(FATAL_ERROR "The variable ORTHANC_FRAMEWORK_VERSION must be set")
  endif()

  if (DEFINED ORTHANC_FRAMEWORK_MAJOR OR
      DEFINED ORTHANC_FRAMEWORK_MINOR OR
      DEFINED ORTHANC_FRAMEWORK_REVISION OR
      DEFINED ORTHANC_FRAMEWORK_MD5)
    message(FATAL_ERROR "Some internal variable has been set")
  endif()

  set(ORTHANC_FRAMEWORK_MD5 "")

  if (NOT DEFINED ORTHANC_FRAMEWORK_BRANCH)
    if (ORTHANC_FRAMEWORK_VERSION STREQUAL "mainline")
      set(ORTHANC_FRAMEWORK_BRANCH "default")
      set(ORTHANC_FRAMEWORK_MAJOR 999)
      set(ORTHANC_FRAMEWORK_MINOR 999)
      set(ORTHANC_FRAMEWORK_REVISION 999)

    else()
      set(ORTHANC_FRAMEWORK_BRANCH "Orthanc-${ORTHANC_FRAMEWORK_VERSION}")

      set(RE "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
      string(REGEX REPLACE ${RE} "\\1" ORTHANC_FRAMEWORK_MAJOR ${ORTHANC_FRAMEWORK_VERSION})
      string(REGEX REPLACE ${RE} "\\2" ORTHANC_FRAMEWORK_MINOR ${ORTHANC_FRAMEWORK_VERSION})
      string(REGEX REPLACE ${RE} "\\3" ORTHANC_FRAMEWORK_REVISION ${ORTHANC_FRAMEWORK_VERSION})

      if (NOT ORTHANC_FRAMEWORK_MAJOR MATCHES "^[0-9]+$" OR
          NOT ORTHANC_FRAMEWORK_MINOR MATCHES "^[0-9]+$" OR
          NOT ORTHANC_FRAMEWORK_REVISION MATCHES "^[0-9]+$")
        message("Bad version of the Orthanc framework, assuming a pre-release: ${ORTHANC_FRAMEWORK_VERSION}")
        set(ORTHANC_FRAMEWORK_MAJOR 999)
        set(ORTHANC_FRAMEWORK_MINOR 999)
        set(ORTHANC_FRAMEWORK_REVISION 999)
      endif()

      if (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.3.1")
        set(ORTHANC_FRAMEWORK_MD5 "dac95bd6cf86fb19deaf4e612961f378")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.3.2")
        set(ORTHANC_FRAMEWORK_MD5 "d0ccdf68e855d8224331f13774992750")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.4.0")
        set(ORTHANC_FRAMEWORK_MD5 "81e15f34d97ac32bbd7d26e85698835a")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.4.1")
        set(ORTHANC_FRAMEWORK_MD5 "9b6f6114264b17ed421b574cd6476127")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.4.2")
        set(ORTHANC_FRAMEWORK_MD5 "d1ee84927dcf668e60eb5868d24b9394")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.5.0")
        set(ORTHANC_FRAMEWORK_MD5 "4429d8d9dea4ff6648df80ec3c64d79e")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.5.1")
        set(ORTHANC_FRAMEWORK_MD5 "099671538865e5da96208b37494d6718")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.5.2")
        set(ORTHANC_FRAMEWORK_MD5 "8867050f3e9a1ce6157c1ea7a9433b1b")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.5.3")
        set(ORTHANC_FRAMEWORK_MD5 "bf2f5ed1adb8b0fc5f10d278e68e1dfe")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.5.4")
        set(ORTHANC_FRAMEWORK_MD5 "404baef5d4c43e7c5d9410edda8ef5a5")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.5.5")
        set(ORTHANC_FRAMEWORK_MD5 "cfc437e0687ae4bd725fd93dc1f08bc4")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.5.6")
        set(ORTHANC_FRAMEWORK_MD5 "3c29de1e289b5472342947168f0105c0")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.5.7")
        set(ORTHANC_FRAMEWORK_MD5 "e1b76f01116d9b5d4ac8cc39980560e3")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.5.8")
        set(ORTHANC_FRAMEWORK_MD5 "82323e8c49a667f658a3639ea4dbc336")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.6.0")
        set(ORTHANC_FRAMEWORK_MD5 "eab428d6e53f61e847fa360bb17ebe25")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.6.1")
        set(ORTHANC_FRAMEWORK_MD5 "3971f5de96ba71dc9d3f3690afeaa7c0")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.7.0")
        set(ORTHANC_FRAMEWORK_MD5 "ce5f689e852b01d3672bd3d2f952a5ef")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.7.1")
        set(ORTHANC_FRAMEWORK_MD5 "3c171217f930abe80246997bdbcaf7cc")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.7.2")
        set(ORTHANC_FRAMEWORK_MD5 "328f94dcbd78c169655a13f7ad58a2c2")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.7.3")
        set(ORTHANC_FRAMEWORK_MD5 "3f1ba9502ec7c5449971d3b56087bcde")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.7.4")
        set(ORTHANC_FRAMEWORK_MD5 "19fcb7c21876af86546baa048a22c6c0")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.8.0")
        set(ORTHANC_FRAMEWORK_MD5 "f8ec7554ef5d23ea4ce474b1e8214de9")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.8.1")
        set(ORTHANC_FRAMEWORK_MD5 "db094f96399cbe8b9bbdbce34884c220")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.8.2")
        set(ORTHANC_FRAMEWORK_MD5 "8bfa10e66c9931e74111be0bfb1f4548")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.9.0")
        set(ORTHANC_FRAMEWORK_MD5 "cea0b02ce184671eaf1bd668beefbf28")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.9.1")
        set(ORTHANC_FRAMEWORK_MD5 "08eebc66ef93c3b40115c38501db5fbd")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.9.2")
        set(ORTHANC_FRAMEWORK_MD5 "3ea66c09f64aca990016683b6375734e")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.9.3")
        set(ORTHANC_FRAMEWORK_MD5 "9b86e6f00e03278293cd15643cc0233f")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.9.4")
        set(ORTHANC_FRAMEWORK_MD5 "6d5ca4a73ac7d42445041ca79de1624d")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.9.5")
        set(ORTHANC_FRAMEWORK_MD5 "10fc64de1254a095e5d3ed3931f0cfbb")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.9.6")
        set(ORTHANC_FRAMEWORK_MD5 "4b5d05683d747c29b2860ad79d11e62e")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.9.7")
        set(ORTHANC_FRAMEWORK_MD5 "c912bbb860d640d3ae3003b5c9698205")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.10.0")
        set(ORTHANC_FRAMEWORK_MD5 "8610c82d9153f22e929f2110f8f60279")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.10.1")
        set(ORTHANC_FRAMEWORK_MD5 "caf667fc5ea452b3d0c2f70bfd02599c")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.11.0")
        set(ORTHANC_FRAMEWORK_MD5 "962c4a4a706a2ef28b390d8515dd7091")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.11.1")
        set(ORTHANC_FRAMEWORK_MD5 "a39661c406adf22cf574fde290cf4bbf")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.11.2")
        set(ORTHANC_FRAMEWORK_MD5 "ede3de356493a8868545f8cb4b8bc8b5")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.11.3")
        set(ORTHANC_FRAMEWORK_MD5 "f941c0f5771db7616e7b7961026a60e2")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.12.0")
        set(ORTHANC_FRAMEWORK_MD5 "d32a0cde03b6eb603d8dd2b33d38bf1b")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "1.12.1")
        set(ORTHANC_FRAMEWORK_MD5 "8a435140efc8ff4a01d8242f092f21de")

      # Below this point are development snapshots that were used to
      # release some plugin, before an official release of the Orthanc
      # framework was available. Here is the command to be used to
      # generate a proper archive:
      #
      #   $ hg archive /tmp/Orthanc-`hg id -i | sed 's/\+//'`.tar.gz
      #
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "ae0e3fd609df")
        # DICOMweb 1.1 (framework pre-1.6.0)
        set(ORTHANC_FRAMEWORK_MD5 "7e09e9b530a2f527854f0b782d7e0645")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "82652c5fc04f")
        # Stone Web viewer 1.0 (framework pre-1.8.1)
        set(ORTHANC_FRAMEWORK_MD5 "d77331d68917e66a3f4f9b807bbdab7f")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "4a3ba4bf4ba7")
        # PostgreSQL 3.3 (framework pre-1.8.2)
        set(ORTHANC_FRAMEWORK_MD5 "2d82bddf06f9cfe82095495cb3b8abde")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "23ad1b9c7800")
        # For "Toolbox::ReadJson()" and "Toolbox::Write{...}Json()" (pre-1.9.0)
        set(ORTHANC_FRAMEWORK_MD5 "9af92080e57c60dd288eba46ce606c00")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "b2e08d83e21d")
        # WSI 1.1 (framework pre-1.10.0), to remove "-std=c++11"
        set(ORTHANC_FRAMEWORK_MD5 "2eaa073cbb4b44ffba199ad93393b2b1")
      elseif (ORTHANC_FRAMEWORK_VERSION STREQUAL "daf4807631c5")
        # DICOMweb 1.15 (framework pre-1.12.2)
        set(ORTHANC_FRAMEWORK_MD5 "c644aff2817306b3207c98c92e43f35f")
      endif()
    endif()
  endif()

elseif (ORTHANC_FRAMEWORK_SOURCE STREQUAL "path")
  message("Using the Orthanc framework from a path of the filesystem. Assuming mainline version.")
  set(ORTHANC_FRAMEWORK_MAJOR 999)
  set(ORTHANC_FRAMEWORK_MINOR 999)
  set(ORTHANC_FRAMEWORK_REVISION 999)
endif()



##
## Detection of the third-party software
##

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "hg")
  find_program(ORTHANC_FRAMEWORK_HG hg)
  
  if (${ORTHANC_FRAMEWORK_HG} MATCHES "ORTHANC_FRAMEWORK_HG-NOTFOUND")
    message(FATAL_ERROR "Please install Mercurial")
  endif()
endif()


if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "archive" OR
    ORTHANC_FRAMEWORK_SOURCE STREQUAL "web")
  if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
    find_program(ORTHANC_FRAMEWORK_7ZIP 7z 
      PATHS 
      "$ENV{ProgramFiles}/7-Zip"
      "$ENV{ProgramW6432}/7-Zip"
      )

    if (${ORTHANC_FRAMEWORK_7ZIP} MATCHES "ORTHANC_FRAMEWORK_7ZIP-NOTFOUND")
      message(FATAL_ERROR "Please install the '7-zip' software (http://www.7-zip.org/)")
    endif()

  else()
    find_program(ORTHANC_FRAMEWORK_TAR tar)
    if (${ORTHANC_FRAMEWORK_TAR} MATCHES "ORTHANC_FRAMEWORK_TAR-NOTFOUND")
      message(FATAL_ERROR "Please install the 'tar' package")
    endif()
  endif()
endif()



##
## Case of the Orthanc framework specified as a path on the filesystem
##

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "path")
  if (NOT DEFINED ORTHANC_FRAMEWORK_ROOT OR
      ORTHANC_FRAMEWORK_ROOT STREQUAL "")
    message(FATAL_ERROR "The variable ORTHANC_FRAMEWORK_ROOT must provide the path to the sources of Orthanc")
  endif()
  
  if (NOT EXISTS ${ORTHANC_FRAMEWORK_ROOT})
    message(FATAL_ERROR "Non-existing directory: ${ORTHANC_FRAMEWORK_ROOT}")
  endif()
endif()



##
## Case of the Orthanc framework cloned using Mercurial
##

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "hg")
  if (NOT STATIC_BUILD AND NOT ALLOW_DOWNLOADS)
    message(FATAL_ERROR "CMake is not allowed to download from Internet. Please set the ALLOW_DOWNLOADS option to ON")
  endif()

  set(ORTHANC_ROOT ${CMAKE_BINARY_DIR}/orthanc)

  if (EXISTS ${ORTHANC_ROOT})
    message("Updating the Orthanc source repository using Mercurial")
    execute_process(
      COMMAND ${ORTHANC_FRAMEWORK_HG} pull
      WORKING_DIRECTORY ${ORTHANC_ROOT}
      RESULT_VARIABLE Failure
      )    
  else()
    message("Forking the Orthanc source repository using Mercurial")
    execute_process(
      COMMAND ${ORTHANC_FRAMEWORK_HG} clone "https://orthanc.uclouvain.be/hg/orthanc/"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      RESULT_VARIABLE Failure
      )    
  endif()

  if (Failure OR NOT EXISTS ${ORTHANC_ROOT})
    message(FATAL_ERROR "Cannot fork the Orthanc repository")
  endif()

  message("Setting branch of the Orthanc repository to: ${ORTHANC_FRAMEWORK_BRANCH}")

  execute_process(
    COMMAND ${ORTHANC_FRAMEWORK_HG} update -c ${ORTHANC_FRAMEWORK_BRANCH}
    WORKING_DIRECTORY ${ORTHANC_ROOT}
    RESULT_VARIABLE Failure
    )

  if (Failure)
    message(FATAL_ERROR "Error while running Mercurial")
  endif()
endif()



##
## Case of the Orthanc framework provided as a source archive on the
## filesystem
##

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "archive")
  if (NOT DEFINED ORTHANC_FRAMEWORK_ARCHIVE OR
      ORTHANC_FRAMEWORK_ARCHIVE STREQUAL "")
    message(FATAL_ERROR "The variable ORTHANC_FRAMEWORK_ARCHIVE must provide the path to the sources of Orthanc")
  endif()
endif()



##
## Case of the Orthanc framework downloaded from the Web
##

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "web")
  if (DEFINED ORTHANC_FRAMEWORK_URL)
    string(REGEX REPLACE "^.*/" "" ORTHANC_FRAMEMORK_FILENAME "${ORTHANC_FRAMEWORK_URL}")
  else()
    # Default case: Download from the official Web site
    set(ORTHANC_FRAMEMORK_FILENAME Orthanc-${ORTHANC_FRAMEWORK_VERSION}.tar.gz)
    set(ORTHANC_FRAMEWORK_URL "https://orthanc.uclouvain.be/third-party-downloads/orthanc-framework/${ORTHANC_FRAMEMORK_FILENAME}")
  endif()

  set(ORTHANC_FRAMEWORK_ARCHIVE "${CMAKE_SOURCE_DIR}/ThirdPartyDownloads/${ORTHANC_FRAMEMORK_FILENAME}")

  if (NOT EXISTS "${ORTHANC_FRAMEWORK_ARCHIVE}")
    if (NOT STATIC_BUILD AND NOT ALLOW_DOWNLOADS)
      message(FATAL_ERROR "CMake is not allowed to download from Internet. Please set the ALLOW_DOWNLOADS option to ON")
    endif()

    message("Downloading: ${ORTHANC_FRAMEWORK_URL}")

    file(DOWNLOAD
      "${ORTHANC_FRAMEWORK_URL}" "${ORTHANC_FRAMEWORK_ARCHIVE}" 
      SHOW_PROGRESS EXPECTED_MD5 "${ORTHANC_FRAMEWORK_MD5}"
      TIMEOUT 60
      INACTIVITY_TIMEOUT 60
      )
  else()
    message("Using local copy of: ${ORTHANC_FRAMEWORK_URL}")
  endif()  
endif()




##
## Uncompressing the Orthanc framework, if it was retrieved from a
## source archive on the filesystem, or from the official Web site
##

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "archive" OR
    ORTHANC_FRAMEWORK_SOURCE STREQUAL "web")

  if (NOT DEFINED ORTHANC_FRAMEWORK_ARCHIVE OR
      NOT DEFINED ORTHANC_FRAMEWORK_VERSION OR
      NOT DEFINED ORTHANC_FRAMEWORK_MD5)
    message(FATAL_ERROR "Internal error")
  endif()

  if (ORTHANC_FRAMEWORK_MD5 STREQUAL "")
    message(FATAL_ERROR "Unknown release of Orthanc: ${ORTHANC_FRAMEWORK_VERSION}")
  endif()

  file(MD5 ${ORTHANC_FRAMEWORK_ARCHIVE} ActualMD5)

  if (NOT "${ActualMD5}" STREQUAL "${ORTHANC_FRAMEWORK_MD5}")
    message(FATAL_ERROR "The MD5 hash of the Orthanc archive is invalid: ${ORTHANC_FRAMEWORK_ARCHIVE}")
  endif()

  set(ORTHANC_ROOT "${CMAKE_BINARY_DIR}/Orthanc-${ORTHANC_FRAMEWORK_VERSION}")

  if (NOT IS_DIRECTORY "${ORTHANC_ROOT}")
    if (NOT ORTHANC_FRAMEWORK_ARCHIVE MATCHES ".tar.gz$")
      message(FATAL_ERROR "Archive should have the \".tar.gz\" extension: ${ORTHANC_FRAMEWORK_ARCHIVE}")
    endif()
    
    message("Uncompressing: ${ORTHANC_FRAMEWORK_ARCHIVE}")

    if ("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
      # How to silently extract files using 7-zip
      # http://superuser.com/questions/331148/7zip-command-line-extract-silently-quietly

      execute_process(
        COMMAND ${ORTHANC_FRAMEWORK_7ZIP} e -y ${ORTHANC_FRAMEWORK_ARCHIVE}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        RESULT_VARIABLE Failure
        OUTPUT_QUIET
        )
      
      if (Failure)
        message(FATAL_ERROR "Error while running the uncompression tool")
      endif()

      get_filename_component(TMP_FILENAME "${ORTHANC_FRAMEWORK_ARCHIVE}" NAME)
      string(REGEX REPLACE ".gz$" "" TMP_FILENAME2 "${TMP_FILENAME}")

      execute_process(
        COMMAND ${ORTHANC_FRAMEWORK_7ZIP} x -y ${TMP_FILENAME2}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        RESULT_VARIABLE Failure
        OUTPUT_QUIET
        )

    else()
      execute_process(
        COMMAND sh -c "${ORTHANC_FRAMEWORK_TAR} xfz ${ORTHANC_FRAMEWORK_ARCHIVE}"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        RESULT_VARIABLE Failure
        )
    endif()
   
    if (Failure)
      message(FATAL_ERROR "Error while running the uncompression tool")
    endif()

    if (NOT IS_DIRECTORY "${ORTHANC_ROOT}")
      message(FATAL_ERROR "The Orthanc framework was not uncompressed at the proper location. Check the CMake instructions.")
    endif()
  endif()
endif()



##
## Determine the path to the sources of the Orthanc framework
##

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "archive" OR
    ORTHANC_FRAMEWORK_SOURCE STREQUAL "hg" OR
    ORTHANC_FRAMEWORK_SOURCE STREQUAL "web")
  if (NOT DEFINED ORTHANC_ROOT OR
      NOT DEFINED ORTHANC_FRAMEWORK_MAJOR OR
      NOT DEFINED ORTHANC_FRAMEWORK_MINOR OR
      NOT DEFINED ORTHANC_FRAMEWORK_REVISION)
    message(FATAL_ERROR "Internal error in the DownloadOrthancFramework.cmake file")
  endif()

  unset(ORTHANC_FRAMEWORK_ROOT CACHE)

  if ("${ORTHANC_FRAMEWORK_MAJOR}.${ORTHANC_FRAMEWORK_MINOR}.${ORTHANC_FRAMEWORK_REVISION}" VERSION_LESS "1.7.2")
    set(ORTHANC_FRAMEWORK_ROOT "${ORTHANC_ROOT}/Core" CACHE
      STRING "Path to the Orthanc framework source directory")
    set(ENABLE_PLUGINS_VERSION_SCRIPT OFF)
  else()
    set(ORTHANC_FRAMEWORK_ROOT "${ORTHANC_ROOT}/OrthancFramework/Sources" CACHE
      STRING "Path to the Orthanc framework source directory")
  endif()

  unset(ORTHANC_ROOT)
endif()

if (NOT ORTHANC_FRAMEWORK_SOURCE STREQUAL "system")
  if (NOT EXISTS ${ORTHANC_FRAMEWORK_ROOT}/OrthancException.h OR
      NOT EXISTS ${ORTHANC_FRAMEWORK_ROOT}/../Resources/CMake/OrthancFrameworkParameters.cmake)
    message(FATAL_ERROR "Directory not containing the source code of the Orthanc framework: ${ORTHANC_FRAMEWORK_ROOT}")
  endif()
endif()



##
## Case of the Orthanc framework installed as a shared library in a
## GNU/Linux distribution (typically Debian). New in Orthanc 1.7.2.
##

if (ORTHANC_FRAMEWORK_SOURCE STREQUAL "system")
  set(ORTHANC_FRAMEWORK_LIBDIR "" CACHE PATH "")
  set(ORTHANC_FRAMEWORK_USE_SHARED ON CACHE BOOL "Whether to use the shared library or the static library")
  set(ORTHANC_FRAMEWORK_ADDITIONAL_LIBRARIES "" CACHE STRING "Additional libraries to link against, separated by whitespaces, typically needed if using the static library (a common minimal value is \"boost_filesystem boost_iostreams boost_locale boost_regex boost_thread jsoncpp pugixml uuid\")")

  if (CMAKE_SYSTEM_NAME STREQUAL "Windows" AND
      CMAKE_COMPILER_IS_GNUCXX) # MinGW
    set(DYNAMIC_MINGW_STDLIB ON)   # Disable static linking against libc (to throw exceptions)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++")
  endif()
  
  include(CheckIncludeFile)
  include(CheckIncludeFileCXX)
  include(FindPythonInterp)
  include(${CMAKE_CURRENT_LIST_DIR}/Compiler.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/DownloadPackage.cmake)
  include(${CMAKE_CURRENT_LIST_DIR}/AutoGeneratedCode.cmake)
  set(EMBED_RESOURCES_PYTHON ${CMAKE_CURRENT_LIST_DIR}/EmbedResources.py)

  if (ORTHANC_FRAMEWORK_USE_SHARED)
    list(GET CMAKE_FIND_LIBRARY_PREFIXES 0 Prefix)
    list(GET CMAKE_FIND_LIBRARY_SUFFIXES 0 Suffix)
  else()
    list(GET CMAKE_FIND_LIBRARY_PREFIXES 0 Prefix)
    list(GET CMAKE_FIND_LIBRARY_SUFFIXES 1 Suffix)
  endif()

  # The "OrthancFramework" library must be the first one to be included
  if ("${ORTHANC_FRAMEWORK_LIBDIR}" STREQUAL "")
    set(ORTHANC_FRAMEWORK_LIBRARIES ${Prefix}OrthancFramework${Suffix})
  else ()
    set(ORTHANC_FRAMEWORK_LIBRARIES ${ORTHANC_FRAMEWORK_LIBDIR}/${Prefix}OrthancFramework${Suffix})
  endif()

  if (NOT ORTHANC_FRAMEWORK_ADDITIONAL_LIBRARIES STREQUAL "")
    # https://stackoverflow.com/a/5272993/881731
    string(REPLACE " " ";" tmp ${ORTHANC_FRAMEWORK_ADDITIONAL_LIBRARIES})
    list(APPEND ORTHANC_FRAMEWORK_LIBRARIES ${tmp})
  endif()

  # Look for the version of the mandatory dependency JsonCpp (cf. JsonCppConfiguration.cmake)
  if (CMAKE_CROSSCOMPILING)
    set(JSONCPP_INCLUDE_DIR ${ORTHANC_FRAMEWORK_ROOT}/..)
  else()
    find_path(JSONCPP_INCLUDE_DIR json/reader.h
      ${ORTHANC_FRAMEWORK_ROOT}/..
      /usr/include/jsoncpp
      /usr/local/include/jsoncpp
      )
  endif()

  message("JsonCpp include dir: ${JSONCPP_INCLUDE_DIR}")
  include_directories(${JSONCPP_INCLUDE_DIR})

  CHECK_INCLUDE_FILE_CXX(${JSONCPP_INCLUDE_DIR}/json/reader.h HAVE_JSONCPP_H)
  if (NOT HAVE_JSONCPP_H)
    message(FATAL_ERROR "Please install the libjsoncpp-dev package")
  endif()

  # Look for Orthanc framework shared library
  include(CheckCXXSymbolExists)

  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set(ORTHANC_FRAMEWORK_INCLUDE_DIR ${ORTHANC_FRAMEWORK_ROOT})
  else()
    find_path(ORTHANC_FRAMEWORK_INCLUDE_DIR OrthancFramework.h
      /usr/include/orthanc-framework
      /usr/local/include/orthanc-framework
      ${ORTHANC_FRAMEWORK_ROOT}
      )
  endif()

  if (${ORTHANC_FRAMEWORK_INCLUDE_DIR} STREQUAL "ORTHANC_FRAMEWORK_INCLUDE_DIR-NOTFOUND")
    message(FATAL_ERROR "Cannot locate the OrthancFramework.h header")
  endif()
  
  message("Orthanc framework include dir: ${ORTHANC_FRAMEWORK_INCLUDE_DIR}")
  include_directories(${ORTHANC_FRAMEWORK_INCLUDE_DIR})

  if (ORTHANC_FRAMEWORK_USE_SHARED)
    set(CMAKE_REQUIRED_INCLUDES "${ORTHANC_FRAMEWORK_INCLUDE_DIR}")
    set(CMAKE_REQUIRED_LIBRARIES "${ORTHANC_FRAMEWORK_LIBRARIES}")
    
    check_cxx_symbol_exists("Orthanc::InitializeFramework" "OrthancFramework.h" HAVE_ORTHANC_FRAMEWORK)
    if (NOT HAVE_ORTHANC_FRAMEWORK)
      message(FATAL_ERROR "Cannot find the Orthanc framework")
    endif()
    
    unset(CMAKE_REQUIRED_INCLUDES)
    unset(CMAKE_REQUIRED_LIBRARIES)
  endif()
endif()
