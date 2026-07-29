import {readFileSync} from "node:fs";
import path from "node:path";
import {fileURLToPath} from "node:url";

import {Value} from "@sinclair/typebox/value";
import react from "@vitejs/plugin-react";
import {WebuiSettingsSchema} from "@webui/common/schemas/settings";
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
        {
            name: "webui-public-settings",
            configureServer: (server) => {
                const webuiRoot = path.resolve(
                    path.dirname(fileURLToPath(import.meta.url)),
                    "../.."
                );

                server.middlewares.use("/settings.json", (_req, res) => {
                    try {
                        const rawSettings: unknown = JSON.parse(
                            readFileSync(path.join(webuiRoot, "settings.json"), "utf8")
                        );
                        const settings = Value.Parse(WebuiSettingsSchema, rawSettings);
                        res.setHeader("Content-Type", "application/json");
                        res.end(JSON.stringify(settings.public));
                    } catch {
                        res.statusCode = 500;
                        res.end(JSON.stringify({error: "Failed to load WebUI settings."}));
                    }
                });
            },
        },
    ],
    publicDir: "public",
    server: {
        port: 8080,
        proxy: {
            // Below targets should match the server's configuration in
            // `components/webui/.env` (or `.env.local` if overridden).
            "/api/": {
                target: "http://localhost:3000/",
                changeOrigin: true,
            },
            "/streams": {
                target: "http://localhost:3000/",
                changeOrigin: true,
            },
            "/log-viewer": {
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
});
