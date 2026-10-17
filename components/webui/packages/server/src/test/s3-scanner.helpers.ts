import {readFileSync} from "node:fs";

import {Value} from "@sinclair/typebox/value";
import {
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";
import {
    type WebuiSettings,
    WebuiSettingsSchema,
} from "@webui/common/schemas/settings";
import fastify from "fastify";
import type {Test} from "tap";

import {createS3ScannerRoutes} from "../routes/api/ingest/s3-scanner/routes.js";


/**
 * Creates an enabled deployment without AWS credentials in the WebUI.
 *
 * @return A test deployment configuration.
 */
const enabledSettings = (): WebuiSettings => {
    const source = new URL("../../../../settings.json", import.meta.url);
    const raw: unknown = JSON.parse(readFileSync(source, "utf8"));
    const result = Value.Parse(WebuiSettingsSchema, raw);
    result.public.ClpStorageEngine = CLP_STORAGE_ENGINES.CLP_S;
    result.public.LogsInputType = STORAGE_TYPE.S3;
    result.server.LogIngestorUrl = "http://ingestor.internal:3002";

    return result;
};

/**
 * Creates an isolated route app with a controlled upstream.
 *
 * @param t
 * @param send
 * @param configuration
 * @param timeoutMs
 * @return The injection-ready app.
 */
const appFor = (
    t: Test,
    send: typeof fetch,
    configuration = enabledSettings(),
    timeoutMs = 10_000,
) => {
    const app = fastify();
    app.register(createS3ScannerRoutes(configuration, send, timeoutMs), {prefix: "/scanner"});
    t.teardown(() => app.close());

    return app;
};

export {
    appFor, enabledSettings,
};
