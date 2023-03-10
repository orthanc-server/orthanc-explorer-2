import { defineConfig, resolveBaseUrl } from 'vite'
import { resolve } from 'path'
import vue from '@vitejs/plugin-vue'

// https://vitejs.dev/config/
export default defineConfig({
  assetsInclude: './src/assets',
  base: '',
  plugins: [vue()],
  server: {
    host: true,
    port: 3000
  },
  build: {
    chunkSizeWarningLimit: 1000,
    rollupOptions: {
      input: {
        main: resolve(__dirname, 'index.html'),
        landing: resolve(__dirname, 'token-landing.html'),
        retrieve: resolve(__dirname, 'retrieve-and-view.html')
      }
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
})
