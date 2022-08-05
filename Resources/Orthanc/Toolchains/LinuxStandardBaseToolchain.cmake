# Orthanc - A Lightweight, RESTful DICOM Store
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2022 Osimis S.A., Belgium
# Copyright (C) 2021-2022 Sebastien Jodogne, ICTEAM UCLouvain, Belgium
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


#
# Full build, as used on the BuildBot CIS:
#
#   $ LSB_CC=gcc-4.8 LSB_CXX=g++-4.8 cmake ../OrthancServer/ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=../OrthancFramework/Resources/Toolchains/LinuxStandardBaseToolchain.cmake -DUSE_LEGACY_JSONCPP=ON -DUSE_LEGACY_LIBICU=ON -DBOOST_LOCALE_BACKEND=icu -DENABLE_PKCS11=ON -G Ninja
#
# Or, more lightweight version (without libp11 and ICU):
#
#   $ LSB_CC=gcc-4.8 LSB_CXX=g++-4.8 cmake ../OrthancServer/ -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=../OrthancFramework/Resources/Toolchains/LinuxStandardBaseToolchain.cmake -DUSE_LEGACY_JSONCPP=ON -G Ninja
#

INCLUDE(CMakeForceCompiler)

SET(LSB_PATH $ENV{LSB_PATH} CACHE STRING "")
SET(LSB_CC $ENV{LSB_CC} CACHE STRING "")
SET(LSB_CXX $ENV{LSB_CXX} CACHE STRING "")
SET(LSB_TARGET_VERSION "4.0" CACHE STRING "")

IF ("${LSB_PATH}" STREQUAL "")
  SET(LSB_PATH "/opt/lsb")
ENDIF()

IF (EXISTS ${LSB_PATH}/lib64)
  SET(LSB_TARGET_PROCESSOR "x86_64")
  SET(LSB_LIBPATH ${LSB_PATH}/lib64-${LSB_TARGET_VERSION})
ELSEIF (EXISTS ${LSB_PATH}/lib)
  SET(LSB_TARGET_PROCESSOR "x86")
  SET(LSB_LIBPATH ${LSB_PATH}/lib-${LSB_TARGET_VERSION})
ELSE()
  MESSAGE(FATAL_ERROR "Unable to detect the target processor architecture. Check the LSB_PATH environment variable.")
ENDIF()

SET(LSB_CPPPATH ${LSB_PATH}/include)
SET(PKG_CONFIG_PATH ${LSB_LIBPATH}/pkgconfig/)

# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION LinuxStandardBase)
SET(CMAKE_SYSTEM_PROCESSOR ${LSB_TARGET_PROCESSOR})

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER ${LSB_PATH}/bin/lsbcc)

if (${CMAKE_VERSION} VERSION_LESS "3.6.0") 
  CMAKE_FORCE_CXX_COMPILER(${LSB_PATH}/bin/lsbc++ GNU)
else()
  SET(CMAKE_CXX_COMPILER ${LSB_PATH}/bin/lsbc++)
endif()

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH ${LSB_PATH})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)

SET(CMAKE_CROSSCOMPILING OFF)


SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --lsb-target-version=${LSB_TARGET_VERSION} -I${LSB_PATH}/include" CACHE INTERNAL "" FORCE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --lsb-target-version=${LSB_TARGET_VERSION} -nostdinc++ -I${LSB_PATH}/include -I${LSB_PATH}/include/c++ -I${LSB_PATH}/include/c++/backward" CACHE INTERNAL "" FORCE)
SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --lsb-target-version=${LSB_TARGET_VERSION} -L${LSB_LIBPATH} --lsb-besteffort" CACHE INTERNAL "" FORCE)
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --lsb-target-version=${LSB_TARGET_VERSION} -L${LSB_LIBPATH} --lsb-besteffort" CACHE INTERNAL "" FORCE)

if (NOT "${LSB_CXX}" STREQUAL "")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --lsb-cxx=${LSB_CXX}")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --lsb-cxx=${LSB_CXX}")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --lsb-cxx=${LSB_CXX}")
endif()

if (NOT "${LSB_CC}" STREQUAL "")
  SET(CMAKE_C_FLAGS "${CMAKE_CC_FLAGS} --lsb-cc=${LSB_CC}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --lsb-cc=${LSB_CC}")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} --lsb-cc=${LSB_CC}")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --lsb-cc=${LSB_CC}")
endif()

