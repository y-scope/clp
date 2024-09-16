import path from "node:path";
import process from "node:process";
import {fileURLToPath} from "node:url";

import {fastifyStatic} from "@fastify/static";

import settings from "../../settings.json" with {type: "json"};


/**
 * Creates static files serving routes.
 *
 * @param {import("fastify").FastifyInstance} fastify
 * @param {import("fastify").FastifyPluginOptions} options
 */
const routes = async (fastify, options) => {
    const filename = fileURLToPath(import.meta.url);
    const dirname = path.dirname(filename);
    const rootDirname = path.resolve(dirname, "../..");

    let irFilesDir = settings.IrFilesDir;
    if (false === path.isAbsolute(irFilesDir)) {
        irFilesDir = path.resolve(rootDirname, irFilesDir);
    }
    await fastify.register(fastifyStatic, {
        prefix: "/ir",
        root: irFilesDir,
    });

    let logViewerDir = settings.LogViewerDir;
    if (false === path.isAbsolute(logViewerDir)) {
        logViewerDir = path.resolve(rootDirname, logViewerDir);
    }
    await fastify.register(fastifyStatic, {
        prefix: "/log-viewer",
        root: logViewerDir,
        decorateReply: false,
    });

    if ("production" === process.env.NODE_ENV) {
        // In the development environment, we expect the client to use a separate webserver that
        // supports live reloading.
        let clientDir = settings.ClientDir;
        if (false === path.isAbsolute(clientDir)) {
            clientDir = path.resolve(rootDirname, settings.ClientDir);
        }

        await fastify.register(fastifyStatic, {
            prefix: "/",
            root: clientDir,
            decorateReply: false,
        });
    }
};

export default routes;
