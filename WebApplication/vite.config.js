import { defineConfig } from 'vite'
import { resolve } from 'path'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  assetsInclude: './src/assets',
  base: '',
  plugins: [vue()],
  
  server: {
    host: true,
    port: 3000,
    
    proxy: {
      // OE2 конфигурация (темы, языки, настройки)
      '/ui/api': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false
      },
      
      // Стандартный Orthanc API (patients, studies, series)
      '/orthanc': {
        target: 'http://localhost:8042',
        changeOrigin: true,
        secure: false,
        rewrite: (path) => path.replace(/^\/orthanc/, '') // /orthanc/patients -> /patients
      }
      
      // ВАЖНО: НЕ проксируем /ui/app - иначе dev server отдаст встроенный OE2 вместо вашего кода!
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
    postcss: {
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