import react from "@vitejs/plugin-react";
import {defineConfig} from "vite";


// https://vite.dev/config/
export default defineConfig({
    plugins: [react()],
    server: {
        host: "0.0.0.0",
	port: 8081,
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
    },
    publicDir: "public",
});
