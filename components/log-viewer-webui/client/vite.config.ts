import react from "@vitejs/plugin-react";
import {defineConfig} from "vite";


// https://vite.dev/config/
export default defineConfig({
    plugins: [react()],
    server: {
        port: 8080,
        proxy: {
            "/query": {
                target: "http://localhost:3000/query",
                changeOrigin: true,
                rewrite: (path) => path.replace(/^\/query/, ""),
            },
        },
    },
    publicDir: "public",
});
