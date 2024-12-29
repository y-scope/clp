import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    port: 8080,
    proxy: {
      '/query': {
        target: 'http://localhost:3000/query',
        changeOrigin: true,
        rewrite: (path) => path.replace(/^\/query/, '')
      }
    }
  },
  publicDir: 'public',
})
