import path from "node:path";
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
    const rootDirname = path.resolve(dirname, "../../../..");

    let streamFilesDir = settings.StreamFilesDir;

    // Register /streams only if `streamFilesDir` is set (i.e., FS support is enabled in the
    // package)
    // Disable no-unnecessary-condition since linter doesn't understand that settings
    // values are not hardcoded.
    // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
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

    let logViewerDir = settings.LogViewerDir;
    if (false === path.isAbsolute(logViewerDir)) {
        logViewerDir = path.resolve(rootDirname, logViewerDir);
    }
    await fastify.register(fastifyStatic, {
        prefix: "/log-viewer",
        root: logViewerDir,
        decorateReply: false,
    });

    let clientDir = settings.ClientDir;
    if (false === path.isAbsolute(clientDir)) {
        clientDir = path.resolve(rootDirname, settings.ClientDir);
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
