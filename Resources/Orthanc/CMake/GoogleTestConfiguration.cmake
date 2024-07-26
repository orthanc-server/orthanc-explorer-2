# Orthanc - A Lightweight, RESTful DICOM Store
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2023 Osimis S.A., Belgium
# Copyright (C) 2024-2024 Orthanc Team SRL, Belgium
# Copyright (C) 2021-2024 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


if (USE_GOOGLE_TEST_DEBIAN_PACKAGE)
  find_path(GOOGLE_TEST_DEBIAN_SOURCES_DIR
    NAMES src/gtest-all.cc
    PATHS
    ${CROSSTOOL_NG_IMAGE}/usr/src/gtest
    ${CROSSTOOL_NG_IMAGE}/usr/src/googletest/googletest
    PATH_SUFFIXES src
    )

  find_path(GOOGLE_TEST_DEBIAN_INCLUDE_DIR
    NAMES gtest.h
    PATHS
    ${CROSSTOOL_NG_IMAGE}/usr/include/gtest
    )

  message("Path to the Debian Google Test sources: ${GOOGLE_TEST_DEBIAN_SOURCES_DIR}")
  message("Path to the Debian Google Test includes: ${GOOGLE_TEST_DEBIAN_INCLUDE_DIR}")

  set(GOOGLE_TEST_SOURCES
    ${GOOGLE_TEST_DEBIAN_SOURCES_DIR}/src/gtest-all.cc
    )

  include_directories(${GOOGLE_TEST_DEBIAN_SOURCES_DIR})

  if (NOT EXISTS ${GOOGLE_TEST_SOURCES} OR
      NOT EXISTS ${GOOGLE_TEST_DEBIAN_INCLUDE_DIR}/gtest.h)
    message(FATAL_ERROR "Please install the libgtest-dev package")
  endif()

elseif (STATIC_BUILD OR NOT USE_SYSTEM_GOOGLE_TEST)
  set(GOOGLE_TEST_SOURCES_DIR ${CMAKE_BINARY_DIR}/googletest-release-1.8.1)
  set(GOOGLE_TEST_URL "https://orthanc.uclouvain.be/downloads/third-party-downloads/gtest-1.8.1.tar.gz")
  set(GOOGLE_TEST_MD5 "2e6fbeb6a91310a16efe181886c59596")

  DownloadPackage(${GOOGLE_TEST_MD5} ${GOOGLE_TEST_URL} "${GOOGLE_TEST_SOURCES_DIR}")

  include_directories(
    ${GOOGLE_TEST_SOURCES_DIR}/googletest
    ${GOOGLE_TEST_SOURCES_DIR}/googletest/include
    ${GOOGLE_TEST_SOURCES_DIR}
    )

  set(GOOGLE_TEST_SOURCES
    ${GOOGLE_TEST_SOURCES_DIR}/googletest/src/gtest-all.cc
    )

  # https://code.google.com/p/googletest/issues/detail?id=412
  if (MSVC) # VS2012 does not support tuples correctly yet
    add_definitions(/D _VARIADIC_MAX=10)
  endif()
  
  if ("${CMAKE_SYSTEM_VERSION}" STREQUAL "LinuxStandardBase")
    add_definitions(-DGTEST_HAS_CLONE=0)
  endif()
  
  source_group(ThirdParty\\GoogleTest REGULAR_EXPRESSION ${GOOGLE_TEST_SOURCES_DIR}/.*)

else()
  include(FindGTest)
  if (NOT GTEST_FOUND)
    message(FATAL_ERROR "Unable to find GoogleTest")
  endif()

  include_directories(${GTEST_INCLUDE_DIRS})

  # The variable GTEST_LIBRARIES contains the shared library of
  # Google Test, create an alias for more uniformity
  set(GOOGLE_TEST_LIBRARIES ${GTEST_LIBRARIES})
endif()
