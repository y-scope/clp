import react from "@vitejs/plugin-react";
import {defineConfig} from "vite";


// https://vite.dev/config/
export default defineConfig({
    base: "./",
    plugins: [react()],
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
        },
    },
});
