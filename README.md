# Orthanc Explorer 2

This [Orthanc](https://www.orthanc-server.com) plugin implements a new 
user interface for Orthanc to replace the Orthanc Explorer.

## Installation

Binaries are available:
- [here on Github](https://github.com/orthanc-server/orthanc-explorer-2/releases), you'll find the plugin for
  - Windows 64bits
  - Ubuntu 20.04
  - MacOS (Universal binary)
- in the Windows Installers (to come mid-May 2022),
  the plugin is enabled but, right now, the legacy Orthanc Explorer 
  remains the default UI.  Note that, in a future release, OE2 will become the default UI for Orthanc in the Windows Installers.
- in the osimis/orthanc Docker images (to come mid-May 2022),
  the plugin is enabled but, right now, the legacy Orthanc Explorer 
  remains the default UI.  Note that, in a future release, OE2 will become the default UI for Orthanc in the Windows Installers.

The binaries must be copied next to other Orthanc plugins (Windows: `C:\\Program Files\\Orthanc Server\\Plugins`, ubuntu: `/usr/share/orthanc/plugins/`).  You should also possible make sure that the plugin is listed in the `"Plugins"` Orthanc configuration. 

## Configuration

Like any other Orthanc plugins, Orthanc Explorer 2 is configured through
a json [configuration file](Plugin/DefaultConfiguration.json) that is provided to Orthanc at startup.

At minimum, you should provide this configuration file:
```json
{
    "OrthancExplorer2" {
        "Enable": true,
        "IsDefaultOrthancUI": false
    }
}
```

If you are using Docker, the easiest way to try the new Orthanc Explorer 2 is to run this command and then open a browser in http://localhost:8042/ (login: orthanc, pwd: orthanc)

```shell
docker pull osimis/orthanc:orthanc-mainline
docker run -p 8042:8042 --env OE2_ENABLED=true  osimis/orthanc:orthanc-mainline
```


## Development

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

To run the UI together with Orthanc on http://localhost:9999/oe2/app/

in a shell:
```shell
cd WebApplication
npm run dev
``` 

in another shell:
```shell
sudo ./scripts/start-nginx.sh
``` 

This assumes you have an orhtanc listening on localhost:8044.  Edit scripts/nginx-dev.conf if needed.

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
