import path from "node:path";
import process from "node:process";
import {fileURLToPath} from "node:url";

import {fastifyStatic} from "@fastify/static";
import {FastifyPluginAsync} from "fastify";

import settings from "../../settings.json" with {type: "json"};

/**
 * Creates static files serving routes.
 *
 * @param fastify
 */
const routes: FastifyPluginAsync = async (fastify) => {
    const filename = fileURLToPath(import.meta.url);
    const dirname = path.dirname(filename);
    const rootDirname = path.resolve(dirname, "../..");

    // Resolve absolute path for stream files
    let streamFilesDir = settings.StreamFilesDir;
    if (!path.isAbsolute(streamFilesDir)) {
        streamFilesDir = path.resolve(rootDirname, streamFilesDir);
    }
    await fastify.register(fastifyStatic, {
        prefix: "/streams",
        root: streamFilesDir,
    });

    // Resolve absolute path for log viewer files
    let logViewerDir = settings.LogViewerDir;
    if (!path.isAbsolute(logViewerDir)) {
        logViewerDir = path.resolve(rootDirname, logViewerDir);
    }
    await fastify.register(fastifyStatic, {
        prefix: "/log-viewer",
        root: logViewerDir,
        index: false,
        decorateReply: false,
        wildcard: false,
    });

    if (process.env.NODE_ENV === "production") {
        // Resolve absolute path for client files (React build)
        let clientDir = settings.ClientDir;
        if (!path.isAbsolute(clientDir)) {
            clientDir = path.resolve(rootDirname, clientDir);
        }

        await fastify.register(fastifyStatic, {
            prefix: "/",
            root: clientDir,
            decorateReply: false,
            wildcard: false,
        });

        fastify.get('/streamFile', function (_, reply) {
            reply.sendFile('index.html', clientDir);
        });
    }
};

export default routes;
