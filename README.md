# Orthanc Explorer 2

This [Orthanc](https://www.orthanc-server.com) plugin implements a new 
user interface for Orthanc to replace the Orthanc Explorer.

## Installation

Binaries are available:
- [here on Github](https://github.com/orthanc-server/orthanc-explorer-2/releases), you'll find the plugin for
  - Windows 64bits
  - Ubuntu 20.04
  - MacOS (Universal binary)
- in the [Windows Installers](https://orthanc.osimis.io/win-installer/OrthancInstaller-Win64-latest.exe) (64bits version only),
  the plugin is enabled but, right now, the legacy Orthanc Explorer 
  remains the default UI.  Note that, in a future release, OE2 will become the default UI for Orthanc in the Windows Installers.
- in the [osimis/orthanc](https://hub.docker.com/r/osimis/orthanc) Docker images,
  the plugin is enabled but, right now, the legacy Orthanc Explorer 
  remains the default UI.  Note that, in a future release, OE2 will become the default UI for Orthanc in the Windows Installers.

The binaries must be copied next to other Orthanc plugins (Windows: `C:\\Program Files\\Orthanc Server\\Plugins`, ubuntu: `/usr/share/orthanc/plugins/`).  You should also possible make sure that the plugin is listed in the `"Plugins"` Orthanc configuration. 

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

If you are using Docker, the easiest way to try the new Orthanc Explorer 2 is to run this command and then open a browser in http://localhost:8042/ (login: orthanc, pwd: orthanc)

```shell
docker pull osimis/orthanc:orthanc-mainline
docker run -p 8042:8042 --env OE2_ENABLED=true  osimis/orthanc:orthanc-mainline
```


## Development

Prerequisites:
```
apt install nginx
```

This project has been bootstrapped with:

```shell
npm init vite@latest orthanc-explorer-2 -- --template vue
```

To bootstrap it on your machine:

```shell
cd WebApplication
npm install
npm run dev
```

To run the UI together with Orthanc on http://localhost:9999/ui/app/

in a shell:
```shell
cd WebApplication
npm run dev
``` 

in another shell:
```shell
sudo ./scripts/start-nginx.sh
``` 

This assumes you have an orhtanc listening on localhost:8043.  Edit scripts/nginx-dev.conf if needed.

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
cmake -DALLOW_DOWNLOADS=ON -DCMAKE_BUILD_TYPE:STRING=Release -DUSE_SYSTEM_ORTHANC_SDK=OFF /sources/orthanc-explorer-2/
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
LSB_CC=gcc-4.8 LSB_CXX=g++-4.8 cmake -DCMAKE_TOOLCHAIN_FILE=../Resources/Orthanc/Toolchains/LinuxStandardBaseToolchain.cmake -DALLOW_DOWNLOADS=ON -DSTATIC_BUILD=ON -DUSE_LEGACY_JSONCPP=ON ..
make -j4
```

Pre-compiled LSB binaries can be found at: https://lsb.orthanc-server.com/plugin-orthanc-explorer-2/

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
cmake .. -DCMAKE_BUILD_TYPE=Debug -DORTHANC_FRAMEWORK_SOURCE=system -DUSE_SYSTEM_GOOGLE_TEST=OFF -DALLOW_DOWNLOADS=ON -DUSE_SYSTEM_ORTHANC_SDK=OFF
make -j4
```


## Releasing

```
git tag -a 0.2.0 -m 0.2.0
git push --tags
git push
```

### Contributions

Feel free to fork this project, modify it and submit pull requests.  However,
you'll have to sign a CLA to transfer the ownership of your contribution to
Osimis before your contributions can be reviewed and accepted (CLA process to come).
