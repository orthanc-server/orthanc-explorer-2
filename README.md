# Orthanc Explorer 2

This [Orthanc](https://www.orthanc-server.com) plugin implements a new 
user interface for Orthanc to replace the Orthanc Explorer.

## Installation

Binaries are available for multiple platforms:
- OE2 is the default UI when using the [Windows Installers](https://orthanc.uclouvain.be/downloads/windows-64/installers/index.html),
- OE2 is the default UI when using the [orthancteam/orthanc](https://hub.docker.com/r/orthancteam/orthanc) Docker images,
- [Linux Standard Base](https://orthanc.uclouvain.be/downloads/linux-standard-base/orthanc-explorer-2/index.html) binaries should work on most of Linux distributions.

## Configuration

Like any other Orthanc plugins, Orthanc Explorer 2 is configured through
a json [configuration file](Plugin/DefaultConfiguration.json) that is provided to Orthanc at startup.

At minimum, you should provide this configuration file:
```json
{
    "Plugins": [ "." ],
    "OrthancExplorer2": {
        "Enable": true,
        "IsDefaultOrthancUI": false
    }
}
```

Using this minimal configuration file, Orthanc Explorer 2 is
accessible at http://localhost:8042/ui/app/ . If `IsDefaultOrthancUI`
is set to `true`, Orthanc Explorer 2 will replace the built-in Orthanc
Explorer.

If you are using Docker, the easiest way to try the new Orthanc Explorer 2 is to run this command and then open a browser in http://localhost:8042/ui/app/ 

```shell
docker pull orthancteam/orthanc:latest
docker run -p 8042:8042 -e ORTHANC__AUTHENTICATION_ENABLED=false orthancteam/orthanc:latest
```


## Front-end development

Prerequisites:

- install `nodejs` version 14 or higher and `npm` version 6 or higher

Then, to continuously build and serve the front-end code on your machine, in a shell, type:

```shell
cd WebApplication
npm install
npm run dev
```
Npm will then serve the `/ui/app/` static code (HTML/JS) and run a reverse proxy to Orthanc (assuming it is running on localhost:8043).

Then, launch an Orthanc with the OE2 plugin already enabled and listening on localhost:8043.  This can be achieved with by typing this command in another shell:

```shell
docker run -p 8043:8042 -e ORTHANC__AUTHENTICATION_ENABLED=false orthancteam/orthanc:latest
```
This Orthanc will serve the `/ui/api/` routes and all other Orthanc API routes..

You may then open http://localhost:3000/ui/app/ to view and debug your current front-end code.  As soon as you modify a `WebApplication` source file, the UI shall reload automatically in the browser.


## Compilation

Prerequisites to build the frontend: you need `nodejs` version 14 or higher and `npm` version 6 or higher.

To build the frontend:

```shell
cd WebApplication
npm install
npm run build
```

And then, to build the C++ plugin:
```
cd /build
cmake -DWEBAPP_DIST_SOURCE=LOCAL -DALLOW_DOWNLOADS=ON -DCMAKE_BUILD_TYPE:STRING=Release -DUSE_SYSTEM_ORTHANC_SDK=OFF /sources/orthanc-explorer-2/
make -j 4
```

### LSB (Linux Standard Base)

Here are the build instructions for LSB:

```
cd WebApplication
npm install
npm run build
cd ..
mkdir build-lsb
cd build-lsb
LSB_CC=gcc-4.8 LSB_CXX=g++-4.8 cmake -DCMAKE_TOOLCHAIN_FILE=../Resources/Orthanc/Toolchains/LinuxStandardBaseToolchain.cmake -DALLOW_DOWNLOADS=ON -DSTATIC_BUILD=ON -DUSE_LEGACY_JSONCPP=ON -DWEBAPP_DIST_SOURCE=WEB ..
make -j4
```

Pre-compiled LSB binaries can be found at: https://orthanc.uclouvain.be/downloads/linux-standard-base/orthanc-explorer-2/index.html

### Linking against system-wide Orthanc Framework

Here are the build instructions to dynamic link against the
system-wide Orthanc Framework:

```
cd WebApplication
npm install
npm run build
cd ..
mkdir build-system
cd build-system
cmake .. -DCMAKE_BUILD_TYPE=Debug -DORTHANC_FRAMEWORK_SOURCE=system -DUSE_SYSTEM_GOOGLE_TEST=OFF -DALLOW_DOWNLOADS=ON -DUSE_SYSTEM_ORTHANC_SDK=OFF -DWEBAPP_DIST_SOURCE=LOCAL
make -j4
```


## Releasing

```
git tag -a 0.2.0 -m 0.2.0
git push --tags
git push
```

### Contributions

Feel free to fork this project, modify it and submit pull requests.
