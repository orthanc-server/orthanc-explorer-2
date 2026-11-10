# This docker file builds the OE2 plugin that we can use for automated tests
# docker build -t orthancteam/orthanc-pre-release:latest-oe2-for-tests .

FROM orthancteam/orthanc-builder-base:resolute-20260707-unstable AS build-oe2

# Install dependencies
RUN apt-get update && \
    apt-get install -y curl ca-certificates gnupg && \
    curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | gpg --dearmor -o /usr/share/keyrings/nodesource.gpg && \
    echo "deb [signed-by=/usr/share/keyrings/nodesource.gpg] https://deb.nodesource.com/node_24.x nodistro main" | tee /etc/apt/sources.list.d/nodesource.list && \
    apt-get update && \
    apt-get install -y nodejs

RUN mkdir -p /sources/WebApplication

COPY ./WebApplication/package*.json /sources/WebApplication

WORKDIR /sources/WebApplication

RUN npm install

COPY . /sources

RUN npm run build

WORKDIR /build

RUN cmake -DALLOW_DOWNLOADS=ON -DCMAKE_BUILD_TYPE:STRING=Release -DUSE_SYSTEM_ORTHANC_SDK=OFF -DPLUGIN_VERSION=0.0.0 -DTHIRD_PARTY_DOWNLOADS_ROOT_URL=https://public-files.orthanc.team/third-party-downloads -DORTHANC_SOURCES_DOWNLOADS_ROOT_URL=https://public-files.orthanc.team/third-party-downloads/orthanc-framework /sources
RUN make -j 8

FROM orthancteam/orthanc-pre-release:master-unstable

COPY --from=build-oe2 --chown=orthanc:orthanc --chmod=755 /build/libOrthancExplorer2.so /usr/share/orthanc/plugins-available/