import fastifyHttpProxy from "@fastify/http-proxy";
import fp from "fastify-plugin";

import {serverSettings} from "../../settings.js";


/**
 * Reverse proxy that forwards `/api/v1/*` requests to the API server. The `/api/v1` prefix is
 * stripped before forwarding since the API server's routes live at its root.
 */
export default fp(
    async (fastify) => {
        if (null === serverSettings.ApiServerUrl) {
            fastify.log.error(
                "ApiServerUrl is not configured; /api/v1 will not be proxied and every " +
                "metadata, compression, file-listing, and stream-extraction request will fail.",
            );

            return;
        }

        await fastify.register(fastifyHttpProxy, {
            prefix: "/api/v1",
            upstream: serverSettings.ApiServerUrl,
            undici: {
                bodyTimeout: 0,
                headersTimeout: 0,
            },
        });
    },
);
