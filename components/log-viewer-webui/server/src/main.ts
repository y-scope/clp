// Reference: https://github.com/fastify/demo/blob/main/src/server.ts

import closeWithGrace from "close-with-grace";
import fastify from "fastify";
import fp from "fastify-plugin";

import serviceApp from "./fastify-v2/app.js";


const DEFAULT_FASTIFY_CLOSE_GRACE_DELAY = 500;

/**
 * Generates logger configuration options based on the environment.
 *
 * @return Logger options for Fastify.
 */
const getLoggerOptions = () => {
    // Only if the program is running in an interactive terminal.
    if (process.stdout.isTTY) {
        return {
            level: process.env.LOG_LEVEL ?? "info",
            transport: {
                target: "pino-pretty",
                options: {
                    ignore: "pid,hostname",
                },
            },
        };
    }

    return {level: process.env.LOG_LEVEL ?? "info"};
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
        {delay: Number(DEFAULT_FASTIFY_CLOSE_GRACE_DELAY)},
        async ({err}) => {
            if (err) {
                app.log.error(err);
            }

            await app.close();
        }
    );

    await app.ready();

    try {
        await app.listen({host: app.config.HOST, port: app.config.PORT});
    } catch (err) {
        app.log.error(err);
        process.exit(1);
    }
};

await init();
