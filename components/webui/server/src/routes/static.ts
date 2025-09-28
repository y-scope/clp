import path from "node:path";
import {fileURLToPath} from "node:url";

import fastifyStatic from "@fastify/static";
import {FastifyPluginAsync} from "fastify";

import settings from "../../settings.json" with {type: "json"};


const CACHE_CONTROL_HEADER_KEY = "Cache-Control";
const CACHE_CONTROL_HEADER_VALUE_NO_CACHE = "public, max-age=0";

// Cache all other static files for 1 year without revalidation.
const CACHE_CONTROL_HEADER_VALUE_LONG_TERM_CACHE = "public, max-age=31536000, immutable";


/**
 * Configures `Cache-Control` header for static files to reduce network traffic.
 *
 * @param res
 * @param reqPath
 * @param extraNoCachePaths
 */
const setCacheHeaders = (
    res: Parameters<NonNullable<fastifyStatic.FastifyStaticOptions["setHeaders"]>>[0],
    reqPath: string,
    extraNoCachePaths: string[] = []
): void => {
    const noCachePaths = [
        "/index.html",
        ...extraNoCachePaths,
    ];

    if (noCachePaths.some((noCachePath) => reqPath.endsWith(noCachePath))) {
        res.setHeader(CACHE_CONTROL_HEADER_KEY, CACHE_CONTROL_HEADER_VALUE_NO_CACHE);

        return;
    }

    res.setHeader(CACHE_CONTROL_HEADER_KEY, CACHE_CONTROL_HEADER_VALUE_LONG_TERM_CACHE);
};

/**
 * Creates static files serving routes.
 *
 * @param fastify
 */
const routes: FastifyPluginAsync = async (fastify) => {
    const filename = fileURLToPath(import.meta.url);
    const dirname = path.dirname(filename);
    const rootDirname = path.resolve(dirname, "../../..");

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
        decorateReply: false,
        prefix: "/log-viewer",
        root: logViewerDir,

        // Prevent fastify-static from adding its own cache headers and provide our own.
        cacheControl: false,
        setHeaders: (res, reqPath) => {
            setCacheHeaders(res, reqPath);
        },
    });

    let clientDir = settings.ClientDir;
    if (false === path.isAbsolute(clientDir)) {
        clientDir = path.resolve(rootDirname, settings.ClientDir);
    }
    await fastify.register(fastifyStatic, {
        decorateReply: true,
        prefix: "/",
        root: clientDir,
        wildcard: false,

        // Prevent fastify-static from adding its own cache headers and provide our own.
        cacheControl: false,
        setHeaders: (res, reqPath) => {
            setCacheHeaders(res, reqPath, ["/settings.json"]);
        },
    });

    // Serve index.html for all unmatched routes in the React Single Page Application (SPA).
    fastify.get("/*", (_, reply) => {
        reply.sendFile("index.html");
    });
};

export default routes;
