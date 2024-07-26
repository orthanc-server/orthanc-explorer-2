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


# This file sets all the compiler-related flags


# Save the current compiler flags to the cache every time cmake configures the project
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "compiler flags" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" CACHE STRING "compiler flags" FORCE)


include(CheckLibraryExists)

if ((CMAKE_CROSSCOMPILING AND NOT
      "${CMAKE_SYSTEM_VERSION}" STREQUAL "CrossToolNg") OR    
    "${CMAKE_SYSTEM_VERSION}" STREQUAL "LinuxStandardBase")
  # Cross-compilation necessarily implies standalone and static build
  SET(STATIC_BUILD ON)
  SET(STANDALONE_BUILD ON)
endif()


if ("${CMAKE_SYSTEM_VERSION}" STREQUAL "LinuxStandardBase")
  # Cache the environment variables "LSB_CC" and "LSB_CXX" for further
  # use by "ExternalProject" in CMake
  SET(CMAKE_LSB_CC $ENV{LSB_CC} CACHE STRING "")
  SET(CMAKE_LSB_CXX $ENV{LSB_CXX} CACHE STRING "")

  # This is necessary to build "Orthanc mainline - Framework LSB
  # Release" on "buildbot-worker-debian11"
  set(LSB_PTHREAD_NONSHARED "${LSB_PATH}/lib64-${LSB_TARGET_VERSION}/libpthread_nonshared.a")
  if (EXISTS ${LSB_PTHREAD_NONSHARED})
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LSB_PTHREAD_NONSHARED}")
  endif()
endif()


if (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-long-long")

  # --std=c99 makes libcurl not to compile
  # -pedantic gives a lot of warnings on OpenSSL 
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -Wno-variadic-macros")

  if (CMAKE_CROSSCOMPILING)
    # http://stackoverflow.com/a/3543845/881731
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -I<CMAKE_CURRENT_SOURCE_DIR> <SOURCE> <OBJECT>")
  endif()

elseif (MSVC)
  # Use static runtime under Visual Studio
  # http://www.cmake.org/Wiki/CMake_FAQ#Dynamic_Replace
  # http://stackoverflow.com/a/6510446
  foreach(flag_var
    CMAKE_C_FLAGS_DEBUG
    CMAKE_CXX_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE 
    CMAKE_CXX_FLAGS_RELEASE
    CMAKE_C_FLAGS_MINSIZEREL 
    CMAKE_CXX_FLAGS_MINSIZEREL 
    CMAKE_C_FLAGS_RELWITHDEBINFO 
    CMAKE_CXX_FLAGS_RELWITHDEBINFO) 
    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
  endforeach(flag_var)

  # Add /Zm256 compiler option to Visual Studio to fix PCH errors
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm256")

  # New in Orthanc 1.5.5
  if (MSVC_MULTIPLE_PROCESSES)
    # "If you omit the processMax argument in the /MP option, the
    # compiler obtains the number of effective processors from the
    # operating system, and then creates one process per effective
    # processor"
    # https://blog.kitware.com/cmake-building-with-all-your-cores/
    # https://docs.microsoft.com/en-us/cpp/build/reference/mp-build-with-multiple-processes
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
  endif()
    
  add_definitions(
    -D_CRT_SECURE_NO_WARNINGS=1
    -D_CRT_SECURE_NO_DEPRECATE=1
    )

  if (MSVC_VERSION LESS 1600)
    # Starting with Visual Studio >= 2010 (i.e. macro _MSC_VER >=
    # 1600), Microsoft ships a standard-compliant <stdint.h>
    # header. For earlier versions of Visual Studio, give access to a
    # compatibility header.
    # http://stackoverflow.com/a/70630/881731
    # https://en.wikibooks.org/wiki/C_Programming/C_Reference/stdint.h#External_links
    include_directories(${CMAKE_CURRENT_LIST_DIR}/../../Resources/ThirdParty/VisualStudio)
  endif()

  link_libraries(netapi32)
endif()


if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
  # In FreeBSD/OpenBSD, the "/usr/local/" folder contains the ports and need to be imported
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I/usr/local/include")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/include")
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/local/lib")
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L/usr/local/lib")
endif()


if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "kFreeBSD" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR
    ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")

  if (# NOT ${CMAKE_SYSTEM_VERSION} STREQUAL "LinuxStandardBase" AND
      NOT ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD" AND
      NOT ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    # The "--no-undefined" linker flag makes the shared libraries
    # (plugins ModalityWorklists and ServeFolders) fail to compile on
    # OpenBSD, and make the PostgreSQL plugin complain about missing
    # "environ" global variable in FreeBSD.
    #
    # TODO - Furthermore, on Linux Standard Base running on Debian 12,
    # the "-Wl,--no-undefined" seems to break the compilation (added
    # after Orthanc 1.12.2). This is disabled for now.
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
  endif()

  # Remove the "-rdynamic" option
  # http://www.mail-archive.com/cmake@cmake.org/msg08837.html
  set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
  link_libraries(pthread)

  if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    link_libraries(rt)
  endif()

  if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" AND
      NOT ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    link_libraries(dl)
  endif()

  if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" AND
      NOT ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    # The "--as-needed" linker flag is not available on FreeBSD and OpenBSD
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--as-needed")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
  endif()

  if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" AND
      NOT ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
    # FreeBSD/OpenBSD have just one single interface for file
    # handling, which is 64bit clean, so there is no need to define macro
    # for LFS (Large File Support).
    # https://ohse.de/uwe/articles/lfs.html
    add_definitions(
      -D_LARGEFILE64_SOURCE=1 
      -D_FILE_OFFSET_BITS=64
      )
  endif()

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  if (MSVC)
    message("MSVC compiler version = " ${MSVC_VERSION} "\n")
    # Starting Visual Studio 2013 (version 1800), it is not possible
    # to target Windows XP anymore
    if (MSVC_VERSION LESS 1800)
      add_definitions(
        -DWINVER=0x0501
        -D_WIN32_WINNT=0x0501
        )
    endif()
  else()
    add_definitions(
      -DWINVER=0x0501
      -D_WIN32_WINNT=0x0501
      )
  endif()

  add_definitions(
    -D_CRT_SECURE_NO_WARNINGS=1
    )
  link_libraries(rpcrt4 ws2_32 iphlpapi)  # "iphlpapi" is for "SystemToolbox::GetMacAddresses()"

  if (CMAKE_COMPILER_IS_GNUCXX)
    # Some additional C/C++ compiler flags for MinGW
    SET(MINGW_NO_WARNINGS "-Wno-unused-function -Wno-unused-variable")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MINGW_NO_WARNINGS} -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MINGW_NO_WARNINGS}")

    if (DYNAMIC_MINGW_STDLIB)
    else()
      # This is a patch for MinGW64
      SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-multiple-definition -static-libgcc -static-libstdc++")
      SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--allow-multiple-definition -static-libgcc -static-libstdc++")
    endif()

    CHECK_LIBRARY_EXISTS(winpthread pthread_create "" HAVE_WIN_PTHREAD)
    if (HAVE_WIN_PTHREAD)
      if (DYNAMIC_MINGW_STDLIB)
      else()
        # This line is necessary to compile with recent versions of MinGW,
        # otherwise "libwinpthread-1.dll" is not statically linked.
        SET(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic")
      endif()
      add_definitions(-DHAVE_WIN_PTHREAD=1)
    else()
      add_definitions(-DHAVE_WIN_PTHREAD=0)
    endif()
  endif()

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")

  # fix this error that appears with recent compilers on MacOS: boost/mpl/aux_/integral_wrapper.hpp:73:31: error: integer value -1 is outside the valid range of values [0, 3] for this enumeration type [-Wenum-constexpr-conversion]
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-enum-constexpr-conversion")

  add_definitions(
    -D_XOPEN_SOURCE=1
    )
  link_libraries(iconv)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  message("Building using Emscripten (for WebAssembly or asm.js targets)")
  include(${CMAKE_CURRENT_LIST_DIR}/EmscriptenParameters.cmake)
  
elseif (CMAKE_SYSTEM_NAME STREQUAL "Android")

else()
  message("Unknown target platform: ${CMAKE_SYSTEM_NAME}")
  message(FATAL_ERROR "Support your platform here")
endif()


if (DEFINED ENABLE_PROFILING AND ENABLE_PROFILING)
  if (CMAKE_COMPILER_IS_GNUCXX OR
      CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -pg")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
  else()
    message(FATAL_ERROR "Don't know how to enable profiling on your configuration")
  endif()
endif()


if (CMAKE_COMPILER_IS_GNUCXX)
  # "When creating a static library using binutils (ar) and there
  # exist a duplicate object name (e.g. a/Foo.cpp.o, b/Foo.cpp.o), the
  # resulting static library can end up having only one of the
  # duplicate objects. [...] This bug only happens if there are many
  # objects." The trick consists in replacing the "r" argument
  # ("replace") provided to "ar" (as used in CMake < 3.1) by the "q"
  # argument ("quick append"). This is because of the fact that CMake
  # will invoke "ar" several times with several batches of ".o"
  # objects, and using "r" would overwrite symbols defined in
  # preceding batches. https://cmake.org/Bug/view.php?id=14874
  set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> <LINK_FLAGS> q <TARGET> <OBJECTS>")
endif()


# This function defines macro "__ORTHANC_FILE__" as a replacement to
# macro "__FILE__", as the latter leaks the full path of the source
# files in the binaries
# https://stackoverflow.com/questions/8487986/file-macro-shows-full-path
# https://twitter.com/wget42/status/1676877802375634944?s=20
function(DefineSourceBasenameForTarget targetname)
  # Microsoft Visual Studio is extremely slow if using
  # "set_property()", we only enable this feature for gcc and clang
  if (CMAKE_COMPILER_IS_GNUCXX OR
      CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    get_target_property(source_files "${targetname}" SOURCES)
    foreach(sourcefile ${source_files})
      get_filename_component(basename "${sourcefile}" NAME)
      set_property(
        SOURCE "${sourcefile}" APPEND
        PROPERTY COMPILE_DEFINITIONS "__ORTHANC_FILE__=\"${basename}\"")
    endforeach()
  endif()
endfunction()
