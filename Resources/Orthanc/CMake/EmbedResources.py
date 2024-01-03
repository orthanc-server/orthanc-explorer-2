#!/usr/bin/python

# Orthanc - A Lightweight, RESTful DICOM Store
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2017-2024 Osimis S.A., Belgium
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


import sys
import os
import os.path
import pprint
import re

UPCASE_CHECK = True
USE_SYSTEM_EXCEPTION = False
EXCEPTION_CLASS = 'OrthancException'
OUT_OF_RANGE_EXCEPTION = '::Orthanc::OrthancException(::Orthanc::ErrorCode_ParameterOutOfRange)'
INEXISTENT_PATH_EXCEPTION = '::Orthanc::OrthancException(::Orthanc::ErrorCode_InexistentItem)'
NAMESPACE = 'Orthanc.EmbeddedResources'
FRAMEWORK_PATH = None

ARGS = []
for i in range(len(sys.argv)):
    if not sys.argv[i].startswith('--'):
        ARGS.append(sys.argv[i])
    elif sys.argv[i].lower() == '--no-upcase-check':
        UPCASE_CHECK = False
    elif sys.argv[i].lower() == '--system-exception':
        USE_SYSTEM_EXCEPTION = True
        EXCEPTION_CLASS = '::std::runtime_error'
        OUT_OF_RANGE_EXCEPTION = '%s("Parameter out of range")' % EXCEPTION_CLASS
        INEXISTENT_PATH_EXCEPTION = '%s("Unknown path in a directory resource")' % EXCEPTION_CLASS
    elif sys.argv[i].startswith('--namespace='):
        NAMESPACE = sys.argv[i][sys.argv[i].find('=') + 1 : ]
    elif sys.argv[i].startswith('--framework-path='):
        FRAMEWORK_PATH = sys.argv[i][sys.argv[i].find('=') + 1 : ]

if len(ARGS) < 2 or len(ARGS) % 2 != 0:
    print ('Usage:')
    print ('python %s [--no-upcase-check] [--system-exception] [--namespace=<Namespace>] <TargetBaseFilename> [ <Name> <Source> ]*' % sys.argv[0])
    exit(-1)

TARGET_BASE_FILENAME = ARGS[1]
SOURCES = ARGS[2:]

try:
    # Make sure the destination directory exists
    os.makedirs(os.path.normpath(os.path.join(TARGET_BASE_FILENAME, '..')))
except:
    pass


#####################################################################
## Read each resource file
#####################################################################

def CheckNoUpcase(s):
    global UPCASE_CHECK
    if (UPCASE_CHECK and
        re.search('[A-Z]', s) != None):
        raise Exception("Path in a directory with an upcase letter: %s" % s)

resources = {}

counter = 0
i = 0
while i < len(SOURCES):
    resourceName = SOURCES[i].upper()
    pathName = SOURCES[i + 1]

    if not os.path.exists(pathName):
        raise Exception("Non existing path: %s" % pathName)

    if resourceName in resources:
        raise Exception("Twice the same resource: " + resourceName)
    
    if os.path.isdir(pathName):
        # The resource is a directory: Recursively explore its files
        content = {}
        for root, dirs, files in os.walk(pathName):
            dirs.sort()
            files.sort()
            base = os.path.relpath(root, pathName)

            # Fix issue #24 (Build fails on OSX when directory has .DS_Store files):
            # Ignore folders whose name starts with a dot (".")
            if base.find('/.') != -1:
                print('Ignoring folder: %s' % root)
                continue

            for f in files:
                if f.find('~') == -1:  # Ignore Emacs backup files
                    if base == '.':
                        r = f
                    else:
                        r = os.path.join(base, f)

                    CheckNoUpcase(r)
                    r = '/' + r.replace('\\', '/')
                    if r in content:
                        raise Exception("Twice the same filename (check case): " + r)

                    content[r] = {
                        'Filename' : os.path.join(root, f),
                        'Index' : counter
                        }
                    counter += 1

        resources[resourceName] = {
            'Type' : 'Directory',
            'Files' : content
            }

    elif os.path.isfile(pathName):
        resources[resourceName] = {
            'Type' : 'File',
            'Index' : counter,
            'Filename' : pathName
            }
        counter += 1

    else:
        raise Exception("Not a regular file, nor a directory: " + pathName)

    i += 2

#pprint.pprint(resources)


#####################################################################
## Write .h header
#####################################################################

header = open(TARGET_BASE_FILENAME + '.h', 'w')

header.write("""
#pragma once

#include <string>
#include <list>

#if defined(_MSC_VER)
#  pragma warning(disable: 4065)  // "Switch statement contains 'default' but no 'case' labels"
#endif

""")


for ns in NAMESPACE.split('.'):
    header.write('namespace %s {\n' % ns)
    

header.write("""
    enum FileResourceId
    {
""")

isFirst = True
for name in resources:
    if resources[name]['Type'] == 'File':
        if isFirst:
            isFirst = False
        else:    
            header.write(',\n')
        header.write('      %s' % name)

header.write("""
    };

    enum DirectoryResourceId
    {
""")

isFirst = True
for name in resources:
    if resources[name]['Type'] == 'Directory':
        if isFirst:
            isFirst = False
        else:    
            header.write(',\n')
        header.write('      %s' % name)

header.write("""
    };

    const void* GetFileResourceBuffer(FileResourceId id);
    size_t GetFileResourceSize(FileResourceId id);
    void GetFileResource(std::string& result, FileResourceId id);

    const void* GetDirectoryResourceBuffer(DirectoryResourceId id, const char* path);
    size_t GetDirectoryResourceSize(DirectoryResourceId id, const char* path);
    void GetDirectoryResource(std::string& result, DirectoryResourceId id, const char* path);

    void ListResources(std::list<std::string>& result, DirectoryResourceId id);

""")


for ns in NAMESPACE.split('.'):
    header.write('}\n')

header.close()



#####################################################################
## Write the resource content in the .cpp source
#####################################################################

PYTHON_MAJOR_VERSION = sys.version_info[0]

def WriteResource(cpp, item):
    cpp.write('    static const uint8_t resource%dBuffer[] = {' % item['Index'])

    f = open(item['Filename'], "rb")
    content = f.read()
    f.close()

    # http://stackoverflow.com/a/1035360
    pos = 0
    buffer = []  # instead of appending a few bytes at a time to the cpp file, 
                 # we first append each chunk to a list, join it and write it 
                 # to the file.  We've measured that it was 2-3 times faster in python3.
                 # Note that speed is important since if generation is too slow,
                 # cmake might try to compile the EmbeddedResources.cpp file while it is
                 # still being generated !
    for b in content:
        if PYTHON_MAJOR_VERSION == 2:
            c = ord(b[0])
        else:
            c = b

        if pos > 0:
            buffer.append(",")

        if (pos % 16) == 0:
            buffer.append("\n")

        if c < 0:
            raise Exception("Internal error")

        buffer.append("0x%02x" % c)
        pos += 1

    cpp.write("".join(buffer))
    # Zero-size array are disallowed, so we put one single void character in it.
    if pos == 0:
        cpp.write('  0')

    cpp.write('  };\n')
    cpp.write('    static const size_t resource%dSize = %d;\n' % (item['Index'], pos))


cpp = open(TARGET_BASE_FILENAME + '.cpp', 'w')

cpp.write('#include "%s.h"\n' % os.path.basename(TARGET_BASE_FILENAME))

if USE_SYSTEM_EXCEPTION:
    cpp.write('#include <stdexcept>')
elif FRAMEWORK_PATH != None:
    cpp.write('#include "%s/OrthancException.h"' % FRAMEWORK_PATH)
else:
    cpp.write('#include <OrthancException.h>')

cpp.write("""
#include <stdint.h>
#include <string.h>

""")

for ns in NAMESPACE.split('.'):
    cpp.write('namespace %s {\n' % ns)


for name in resources:
    if resources[name]['Type'] == 'File':
        WriteResource(cpp, resources[name])
    else:
        for f in resources[name]['Files']:
            WriteResource(cpp, resources[name]['Files'][f])



#####################################################################
## Write the accessors to the file resources in .cpp
#####################################################################

cpp.write("""
    const void* GetFileResourceBuffer(FileResourceId id)
    {
      switch (id)
      {
""")
for name in resources:
    if resources[name]['Type'] == 'File':
        cpp.write('      case %s:\n' % name)
        cpp.write('        return resource%dBuffer;\n' % resources[name]['Index'])

cpp.write("""
      default:
        throw %s;
      }
    }

    size_t GetFileResourceSize(FileResourceId id)
    {
      switch (id)
      {
""" % OUT_OF_RANGE_EXCEPTION)

for name in resources:
    if resources[name]['Type'] == 'File':
        cpp.write('      case %s:\n' % name)
        cpp.write('        return resource%dSize;\n' % resources[name]['Index'])

cpp.write("""
      default:
        throw %s;
      }
    }
""" % OUT_OF_RANGE_EXCEPTION)



#####################################################################
## Write the accessors to the directory resources in .cpp
#####################################################################

cpp.write("""
    const void* GetDirectoryResourceBuffer(DirectoryResourceId id, const char* path)
    {
      switch (id)
      {
""")

for name in resources:
    if resources[name]['Type'] == 'Directory':
        cpp.write('      case %s:\n' % name)
        isFirst = True
        for path in resources[name]['Files']:
            cpp.write('        if (!strcmp(path, "%s"))\n' % path)
            cpp.write('          return resource%dBuffer;\n' % resources[name]['Files'][path]['Index'])
        cpp.write('        throw %s;\n\n' % INEXISTENT_PATH_EXCEPTION)

cpp.write("""      default:
        throw %s;
      }
    }

    size_t GetDirectoryResourceSize(DirectoryResourceId id, const char* path)
    {
      switch (id)
      {
""" % OUT_OF_RANGE_EXCEPTION)

for name in resources:
    if resources[name]['Type'] == 'Directory':
        cpp.write('      case %s:\n' % name)
        isFirst = True
        for path in resources[name]['Files']:
            cpp.write('        if (!strcmp(path, "%s"))\n' % path)
            cpp.write('          return resource%dSize;\n' % resources[name]['Files'][path]['Index'])
        cpp.write('        throw %s;\n\n' % INEXISTENT_PATH_EXCEPTION)

cpp.write("""      default:
        throw %s;
      }
    }
""" % OUT_OF_RANGE_EXCEPTION)




#####################################################################
## List the resources in a directory
#####################################################################

cpp.write("""
    void ListResources(std::list<std::string>& result, DirectoryResourceId id)
    {
      result.clear();

      switch (id)
      {
""")

for name in resources:
    if resources[name]['Type'] == 'Directory':
        cpp.write('      case %s:\n' % name)
        for path in sorted(resources[name]['Files']):
            cpp.write('        result.push_back("%s");\n' % path)
        cpp.write('        break;\n\n')

cpp.write("""      default:
        throw %s;
      }
    }
""" % OUT_OF_RANGE_EXCEPTION)




#####################################################################
## Write the convenience wrappers in .cpp
#####################################################################

cpp.write("""
    void GetFileResource(std::string& result, FileResourceId id)
    {
      size_t size = GetFileResourceSize(id);
      result.resize(size);
      if (size > 0)
        memcpy(&result[0], GetFileResourceBuffer(id), size);
    }

    void GetDirectoryResource(std::string& result, DirectoryResourceId id, const char* path)
    {
      size_t size = GetDirectoryResourceSize(id, path);
      result.resize(size);
      if (size > 0)
        memcpy(&result[0], GetDirectoryResourceBuffer(id, path), size);
    }
""")


for ns in NAMESPACE.split('.'):
    cpp.write('}\n')

cpp.close()
