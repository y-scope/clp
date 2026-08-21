import fastifyHttpProxy from "@fastify/http-proxy";
import fp from "fastify-plugin";

import {serverSettings} from "../../settings.js";


/**
 * Reverse proxy that forwards `/api/v1/*` requests to the API server. The `/api/v1` prefix is
 * stripped before forwarding since the API server's routes live at its root.
 *
 * `ApiServerUrl` is always set: a deployment without an API server is rejected before the Web UI
 * starts, by `validate_webui_config` in `clp_package_utils.general` and by the Helm chart's
 * `webui-deployment.yaml`.
 */
export default fp(
    async (fastify) => {
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
