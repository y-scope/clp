// Reference: https://github.com/fastify/demo/blob/main/src/app.ts

import path from "node:path";

import {fastifyAutoload} from "@fastify/autoload";
import {FastifyError} from "@fastify/error";
import {
    FastifyInstance,
    FastifyPluginOptions,
} from "fastify";
import {constants} from "http2";


const RATE_LIMIT_MAX_REQUESTS = 3;
const RATE_LIMIT_TIME_WINDOW_MS = 500;

/**
 * Registers all plugins and routes.
 *
 * @param fastify
 * @param opts
 */
// eslint-disable-next-line max-lines-per-function
export default async function serviceApp (
    fastify: FastifyInstance,
    opts: FastifyPluginOptions
) {
    // Option only serves testing purpose. It's used in testing to expose all decorators to the
    // test app. Some decorators may not be exposed in production.
    delete opts.skipOverride;

    // Loads all external plugins. Registered first as application plugins might depend on them.
    await fastify.register(fastifyAutoload, {
        dir: path.join(import.meta.dirname, "plugins/external"),
        options: {...opts},
    });

    // Loads all application plugins.
    fastify.register(fastifyAutoload, {
        dir: path.join(import.meta.dirname, "plugins/app"),
        options: {...opts},
    });

    // Loads all routes.
    fastify.register(fastifyAutoload, {
        autoHooks: true,
        cascadeHooks: true,
        dir: path.join(import.meta.dirname, "routes"),
        options: {...opts},
    });

    fastify.setErrorHandler<FastifyError>((err, request, reply) => {
        fastify.log.error(
            {
                err: err,
                request: {
                    method: request.method,
                    url: request.url,
                    query: request.query,
                    params: request.params,
                },
            },
            "Unhandled error occurred"
        );

        if ("undefined" !== typeof err.statusCode &&
            constants.HTTP_STATUS_INTERNAL_SERVER_ERROR > err.statusCode
        ) {
            reply.code(err.statusCode);

            return err.message;
        }

        reply.internalServerError();

        return {
            message: "Internal Server Error",
        };
    });

    // An attacker could search for valid URLs if 404 error handling is not rate limited.
    fastify.setNotFoundHandler(
        {
            preHandler: fastify.rateLimit({
                max: RATE_LIMIT_MAX_REQUESTS,
                timeWindow: RATE_LIMIT_TIME_WINDOW_MS,
            }),
        },
        (request, reply) => {
            request.log.warn(
                {
                    request: {
                        method: request.method,
                        url: request.url,
                        query: request.query,
                        params: request.params,
                    },
                },
                "Resource not found"
            );

            reply.notFound();

            return {message: "Not Found"};
        }
    );
}
