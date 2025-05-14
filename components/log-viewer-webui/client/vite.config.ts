import react from "@vitejs/plugin-react";
import {defineConfig} from "vite";
import tsconfigPaths from "vite-tsconfig-paths";


// https://vite.dev/config/
export default defineConfig({
    base: "./",
    plugins: [
        react(),
        tsconfigPaths(),
    ],
    publicDir: "public",
    server: {
        port: 8080,
        proxy: {
            "/query/": {
                // Below target should match the server's configuration in
                // `components/log-viewer-webui/server/.env` (or `.env.local` if overridden)
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
