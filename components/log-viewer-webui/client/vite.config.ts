import react from "@vitejs/plugin-react";
import {defineConfig} from "vite";
import tsconfigPaths from 'vite-tsconfig-paths';

// https://vite.dev/config/
export default defineConfig({
    plugins: [react(), tsconfigPaths()],
    server: {
        host: "0.0.0.0",
        port: 8080,
        proxy: {
            "/query/": {
                // Below target should match the server's configuration in
                // `components/log-viewer-webui/server/.env` (or `.env.local` if overridden)
                target: "http://localhost:3001/",
                changeOrigin: true,
            },
            "/socket.io/": {
                target: "ws://localhost:3001/",
                changeOrigin: true,
                ws: true,
            },
        },
        fs: {
            allow: ['..']  // allow serving files from one level up (common folder)
        }
    },
    publicDir: "public",
});