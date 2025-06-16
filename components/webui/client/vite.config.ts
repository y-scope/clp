import path from "node:path";

import react from "@vitejs/plugin-react";
import {defineConfig} from "vite";


// https://vite.dev/config/
export default defineConfig({
    base: "./",
    build: {
        target: "esnext",
    },
    plugins: [
        react(),
    ],
    publicDir: "public",
    resolve: {
        alias: {
            "@common": path.resolve(__dirname, "../common"),
        },
    },
    server: {
        port: 8080,
        proxy: {
            "/query/": {
                // Below target should match the server's configuration in
                // `components/webui/server/.env` (or `.env.local` if overridden)
                target: "http://localhost:3000/",
                changeOrigin: true,
            },
            "/api/": {
                target: "http://localhost:3000/",
                changeOrigin: true,
            },
            "/socket.io/": {
                target: "ws://localhost:3000/",
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
