// Reference: https://github.com/fastify/demo/blob/main/src/server.ts

import closeWithGrace from "close-with-grace";
import fastify from "fastify";
import fp from "fastify-plugin";

import serviceApp from "./fastify-v2/app.js";


const DEFAULT_FASTIFY_CLOSE_GRACE_DELAY = 500;
const DEFAULT_PORT = 3000;

/**
 * Do not use NODE_ENV to determine what logger (or any env related feature) to use
 *
 * @return Logger options for Fastify.
 * @see {@link https://www.youtube.com/watch?v=HMM7GJC5E2o}
 */
const getLoggerOptions = () => {
    // Only if the program is running in an interactive terminal.
    if (process.stdout.isTTY) {
        return {
            level: "info",
            transport: {
                target: "pino-pretty",
                options: {
                    translateTime: "HH:MM:ss Z",
                    ignore: "pid,hostname",
                },
            },
        };
    }

    return {level: process.env.LOG_LEVEL ?? "silent"};
};

const app = fastify({
    logger: getLoggerOptions(),
});

/**
 * Initialize the Fastify server.
 *
 * @return
 */
const init = async (): Promise<void> => {
    // fp must be used to override default error handler.
    app.register(fp(serviceApp));

    closeWithGrace(
        {delay: Number(process.env.FASTIFY_CLOSE_GRACE_DELAY || DEFAULT_FASTIFY_CLOSE_GRACE_DELAY)},
        async ({err}) => {
            if (err) {
                app.log.error(err);
            }

            await app.close();
        }
    );

    await app.ready();

    try {
        await app.listen({port: Number(process.env.PORT || DEFAULT_PORT)});
    } catch (err) {
        app.log.error(err);
        process.exit(1);
    }
};

// eslint-disable-next-line @typescript-eslint/no-floating-promises
init();
