import { defineConfig } from 'vite'
import { resolve } from 'path'
import vue from '@vitejs/plugin-vue'

const targetOrthanc = 'http://localhost:8043'
// https://vitejs.dev/config/
export default defineConfig(({ command }) => ({
  assetsInclude: './src/assets',
  base: (command === 'build' ? '' : '/ui/app/'),
  plugins: [vue()],
  server: {
    host: true,
    port: 3000,
    proxy: {
      // '/ui/api': {
      //   target: targetOrthanc,
      //   changeOrigin: true,
      //   secure: false
      // },
      '^/(ui/api|ui/app/customizable|patients|studies|instances|series|plugins|system|tools|statistics|modalities|dicom-web|osimis-viewer|ohif|stone-webviewer|peers|jobs|transfers|queries|auth|app|volview|changes|wsi|stl|worklists)': {
        target: targetOrthanc,
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path
      },      
    }
  },
  build: {
    chunkSizeWarningLimit: 1000,
    rollupOptions: {
      input: {
        main: resolve(__dirname, 'index.html'),
        landing: resolve(__dirname, 'token-landing.html'),
        retrieve: resolve(__dirname, 'retrieve-and-view.html'),
        inbox: resolve(__dirname, 'inbox.html')
      },
    }
  },
  css: {
    postcss: { // to avoid this warning: https://github.com/vitejs/vite/discussions/5079
      plugins: [
        {
          postcssPlugin: 'internal:charset-removal',
          AtRule: {
            charset: (atRule) => {
              if (atRule.name === 'charset') {
                atRule.remove();
              }
            }
          }
        }
      ]
    }
  }
}))
