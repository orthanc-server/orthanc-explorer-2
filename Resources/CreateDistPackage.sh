#!/bin/bash

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


# This command-line script uses the "npm" tool to populate the "dist"
# folder of Orthanc Explorer 2. It uses Docker to this end, in order
# to be usable on our CIS.

set -ex

if [ -t 1 ]; then
    # TTY is available => use interactive mode
    DOCKER_FLAGS='-i'
fi

ROOT_DIR=`dirname $(readlink -f $0)`/..
IMAGE=orthanc-explorer-2-npm

echo "Creating the distribution of Orthanc Explorer 2"

if [ -e "${ROOT_DIR}/WebApplication/dist/" ]; then
    echo "Target folder is already existing, aborting"
    exit -1
fi

mkdir -p ${ROOT_DIR}/WebApplication/dist/

( cd ${ROOT_DIR}/Resources/CreateDistPackage && \
      docker build --no-cache -t ${IMAGE} . )

docker run -t ${DOCKER_FLAGS} --rm \
       --user $(id -u):$(id -g) \
       -v ${ROOT_DIR}/Resources/CreateDistPackage/build.sh:/source/build.sh:ro \
       -v ${ROOT_DIR}/WebApplication:/source/WebApplication:ro \
       -v ${ROOT_DIR}/WebApplication/dist/:/target:rw \
       ${IMAGE} \
       bash /source/build.sh ${VERSION}
