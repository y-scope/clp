import react from "@vitejs/plugin-react";
import {defineConfig} from "vite";


// https://vite.dev/config/
export default defineConfig({
    base: "./",
    build: {
        target: "esnext",
        rollupOptions: {
            output: {
                manualChunks: {
                    "monaco-editor": ["monaco-editor"],
                },
            },
        },
    },
    plugins: [
        react(),
    ],
    publicDir: "public",
    server: {
        port: 8080,
        proxy: {
            // Below targets should match the server's configuration in
            // `components/webui/server/.env` (or `.env.local` if overridden)
            "/api/": {
                target: "http://localhost:3005/",
                changeOrigin: true,
            },
            "/socket.io/": {
                target: "ws://localhost:3005/",
                changeOrigin: true,
                ws: true,
            },
        },
    },
});
