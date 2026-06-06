import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// The backend runs on :8080. Proxying /api keeps the browser same-origin in
// dev, so there's no CORS dance and the player's stream URLs work as-is.
// Override the target with VITE_API_PROXY when the backend lives elsewhere.
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/api': {
        target: process.env.VITE_API_PROXY || 'http://localhost:8080',
        changeOrigin: true,
      },
    },
  },
})