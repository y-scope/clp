// Reference: https://github.com/fastify/demo/blob/main/test/helper.ts

import path from "node:path";

import {FastifyInstance} from "fastify";
import {build as buildApplication} from "fastify-cli/helper.js";
import type {Test} from "tap";


// eslint-disable-next-line no-warning-comments
// TODO: Setup testing for new webui fastify app in `fastify-v2/app.ts`. Testing will need
// to be split into unit tests and integration tests (require clp package running).
const appPath = path.join(import.meta.dirname, "../app.ts");

/**
 * Provides test configuration options.
 *
 * @return
 */
const config = () => {
    // Register application with fastify-plugin to expose all decorators for testing
    // purposes.
    return {
        skipOverride: true,
    };
};

/**
 * Automatically build and tear down fastify app.
 *
 * @param t The test instance.
 * @return Fastify instance.
 */
const build = async (t: Test): Promise<FastifyInstance> => {
    const argv = [appPath];

    const app = await buildApplication(argv, config()) as FastifyInstance;

    console.log("Application built.");

    t.teardown(async () => {
        console.log("Closing application.");
        await app.close();
    });

    return app;
};

export {build};
