// import { defineConfig } from 'vite'
// import { resolve } from 'path'
// import vue from '@vitejs/plugin-vue'

// // https://vitejs.dev/config/
// export default defineConfig({
//   assetsInclude: './src/assets',
//   base: '',
//   plugins: [vue()],
//   server: {
//     host: true,
//     port: 3000
//   },
//   build: {
//     chunkSizeWarningLimit: 1000,
//     rollupOptions: {
//       input: {
//         main: resolve(__dirname, 'index.html'),
//         landing: resolve(__dirname, 'token-landing.html'),
//         retrieve: resolve(__dirname, 'retrieve-and-view.html'),
//         inbox: resolve(__dirname, 'inbox.html')
//       },
//     }
//   },
//   css: {
//     postcss: { // to avoid this warning: https://github.com/vitejs/vite/discussions/5079
//       plugins: [
//         {
//           postcssPlugin: 'internal:charset-removal',
//           AtRule: {
//             charset: (atRule) => {
//               if (atRule.name === 'charset') {
//                 atRule.remove();
//               }
//             }
//           }
//         }
//       ]
//     }
//   }
// })



import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [
    vue(),
    // {
    //   name: 'configure-server',
    //   configureServer(server) {
    //     server.middlewares.use('/system', (req, res, next) => {
    //       req.url = 'http://localhost:8042/system' + req.url;
    //       next();
    //     });
    //   }
    // }
  ],
  server: {
    port: 3000,
    proxy: {
      // OE2 UI API (конфигурация)
      '/ui/api': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false
      },
      // Orthanc REST API — явное проксирование
      '^/system': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/studies': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/series': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/instances': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/patients': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/changes': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/jobs': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/tools': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/modalities': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/peers': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/plugins': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/statistics': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/worklists': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/auth': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/dicom-web': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/transfers': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },
      '^/queries': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      }
    }
  }
})