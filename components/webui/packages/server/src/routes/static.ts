import path from "node:path";
import {fileURLToPath} from "node:url";

import {fastifyStatic} from "@fastify/static";
import {FastifyPluginAsync} from "fastify";

import {
    publicSettings,
    serverSettings,
} from "../settings.js";


/**
 * Creates static files serving routes.
 *
 * @param fastify
 */
const routes: FastifyPluginAsync = async (fastify) => {
    const filename = fileURLToPath(import.meta.url);
    const dirname = path.dirname(filename);
    const rootDirname = path.resolve(dirname, "../..");

    // Register before other static handlers so this route owns /settings.json.
    fastify.get("/settings.json", () => publicSettings);

    let streamFilesDir = serverSettings.StreamFilesDir;

    // Register /streams only if `streamFilesDir` is set (i.e., FS support is enabled in the
    // package)
    if (null !== streamFilesDir) {
        if (false === path.isAbsolute(streamFilesDir)) {
            streamFilesDir = path.resolve(rootDirname, streamFilesDir);
        }
        await fastify.register(fastifyStatic, {
            prefix: "/streams",
            root: streamFilesDir,
            decorateReply: false,
        });
    }

    let logViewerDir = serverSettings.LogViewerDir;
    if (false === path.isAbsolute(logViewerDir)) {
        logViewerDir = path.resolve(rootDirname, logViewerDir);
    }
    await fastify.register(fastifyStatic, {
        prefix: "/log-viewer",
        root: logViewerDir,
        decorateReply: false,
    });

    let clientDir = serverSettings.ClientDir;
    if (false === path.isAbsolute(clientDir)) {
        clientDir = path.resolve(rootDirname, serverSettings.ClientDir);
    }

    await fastify.register(fastifyStatic, {
        prefix: "/",
        root: clientDir,
        decorateReply: true,
        wildcard: false,
    });

    // Serve index.html for all unmatched routes in the React Single Page Application (SPA).
    fastify.get("/*", (_, reply) => {
        reply.sendFile("index.html");
    });
};

export default routes;
