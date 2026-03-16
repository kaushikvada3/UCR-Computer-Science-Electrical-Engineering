import path from 'node:path'
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

const appBasePath = process.env.VITE_APP_BASE_PATH || '/'

export default defineConfig({
  base: appBasePath,
  plugins: [react()],
  resolve: {
    alias: {
      '@shared': path.resolve(__dirname, '../shared/src'),
    },
  },
  server: {
    port: 5173,
    fs: {
      allow: ['..'],
    },
    proxy: {
      '/api': 'http://localhost:8787',
      '/assets': 'http://localhost:8787',
      '/collab': {
        target: 'ws://localhost:8787',
        ws: true,
      },
    },
  },
})
