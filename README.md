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
