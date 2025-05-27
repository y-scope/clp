import path from "node:path";

import react from "@vitejs/plugin-react";
import {defineConfig} from "vite";


// https://vite.dev/config/
export default defineConfig({
    base: "./",
    plugins: [
        react(),
    ],
    resolve: {
        alias: {
            "@common": path.resolve(__dirname, "../common"),
        },
    },
    publicDir: "public",
    server: {
        port: 8080,
        proxy: {
            "/query/": {
                // Below target should match the server's configuration in
                // `components/log-viewer-webui/server/.env` (or `.env.local` if overridden)
                target: "http://localhost:3001/",
                changeOrigin: true,
            },
            "/api/": {
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
            // allow serving files from one level up (common folder)
            allow: [".."],
        },
    },
});
