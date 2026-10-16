import {createServer} from "node:http";

import {
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";
import {DATASET_NAME_MAX_LEN} from "@webui/common/schemas/compression";
import {isS3ScannerEnabled} from "@webui/common/schemas/ingestion";
import fastify from "fastify";
import {constants} from "http2";
import tap from "tap";

import {createS3ScannerRoutes} from "../routes/api/ingest/s3-scanner/routes.js";
import {
    appFor,
    enabledSettings,
} from "./s3-scanner.helpers.js";


const input = {bucket: "test-bucket", keyPrefix: "logs/", region: "us-east-2"};
const MAX_BODY_BYTES = 16_384;

tap.test("forwards exactly one allowed native request and returns scanner ID", async (t) => {
    const calls: {url: string; options: RequestInit | undefined}[] = [];
    const app = appFor(t, async (url, options) => {
        calls.push({
            url: url instanceof Request ?
                url.url :
                url.toString(),
            options: options,
        });

        return Response.json({id: 27});
    });
    const response = await app.inject({
        method: "POST",
        payload: {...input, dataset: "test_dataset", timestampKey: "timestamp"},
        url: "/scanner",
    });

    t.equal(response.statusCode, constants.HTTP_STATUS_OK);
    t.same(response.json(), {id: 27});
    t.equal(calls.length, 1);
    t.equal(calls[0]?.url, "http://ingestor.internal:3002/s3_scanner");
    t.equal(calls[0]?.options?.method, "POST");
    t.equal(calls[0]?.options?.redirect, "error");
    const body = calls[0]?.options?.body;
    if ("string" !== typeof body) {
        throw new Error("Expected a JSON string request body");
    }
    t.same(JSON.parse(body), {
        bucket_name: "test-bucket",
        dataset: "test_dataset",
        key_prefix: "logs/",
        region: "us-east-2",
        timestamp_key: "timestamp",
        unstructured: false,
    });
});

tap.test("rejects invalid direct API requests without contacting the ingestor", async (t) => {
    let calls = 0;
    const app = appFor(t, async () => {
        calls++;

        return Response.json({id: 1});
    });
    const invalidBodies = [
        {},
        {...input, bucket: ""},
        {...input, bucket: "   "},
        {...input, bucket: 123},
        {...input, region: ""},
        {...input, region: "\t"},
        {...input, keyPrefix: ""},
        {...input, keyPrefix: ["logs/"]},
        {...input, dataset: "bad-name"},
        {...input, dataset: "a".repeat(DATASET_NAME_MAX_LEN + 1)},
        {...input, dataset: null},
        {...input, timestampKey: 123},
        {...input, unstructured: "false"},
        {...input, unstructured: null},
        {...input, endpoint_url: "http://another-service"},
        {...input, aws_authentication: {type: "default"}},
        {...input, scanning_interval_sec: 1},
    ];

    for (const payload of invalidBodies) {
        const response = await app.inject({method: "POST", payload: payload, url: "/scanner"});
        t.equal(response.statusCode, constants.HTTP_STATUS_BAD_REQUEST, JSON.stringify(payload));
        t.type(response.json<{error: string}>().error, "string");
    }
    const tooLarge = await app.inject({
        method: "POST",
        payload: {...input, keyPrefix: "a".repeat(MAX_BODY_BYTES)},
        url: "/scanner",
    });

    t.equal(tooLarge.statusCode, constants.HTTP_STATUS_PAYLOAD_TOO_LARGE);
    t.equal(calls, 0);
});

tap.test("checks every deployment gate independently of AWS key presence", async (t) => {
    let calls = 0;
    const send: typeof fetch = async () => {
        calls++;

        return Response.json({id: 1});
    };

    for (const clpS of [false,
        true]) {
        for (const s3 of [false,
            true]) {
            for (const configured of [false,
                true]) {
                for (const url of [null,
                    "http://ingestor.internal:3002"]) {
                    const configuration = enabledSettings();
                    configuration.public.ClpStorageEngine = clpS ?
                        CLP_STORAGE_ENGINES.CLP_S :
                        CLP_STORAGE_ENGINES.CLP;
                    configuration.public.LogsInputType = s3 ?
                        STORAGE_TYPE.S3 :
                        STORAGE_TYPE.FS;
                    configuration.public.S3ScannerEnabled = configured;
                    configuration.server.LogIngestorUrl = url;
                    const app = appFor(t, send, configuration);
                    const response = await app.inject({
                        method: "POST", payload: input, url: "/scanner",
                    });
                    const enabled = clpS && s3 && configured && null !== url;
                    t.equal(response.statusCode, enabled ?
                        constants.HTTP_STATUS_OK :
                        constants.HTTP_STATUS_SERVICE_UNAVAILABLE);
                    t.equal(isS3ScannerEnabled(configuration.public), clpS && s3 && configured);
                }
            }
        }
    }
    t.equal(calls, 1);
});

tap.test("passes native JSON and text user-input errors through", async (t) => {
    for (const status of [constants.HTTP_STATUS_BAD_REQUEST,
        constants.HTTP_STATUS_CONFLICT,
        constants.HTTP_STATUS_UNPROCESSABLE_ENTITY,
        constants.HTTP_STATUS_TOO_MANY_REQUESTS]) {
        for (const json of [false,
            true]) {
            const message = "Prefix conflicts with an existing scanner";
            const app = appFor(t, async () => new Response(json ?
                JSON.stringify({error: message}) :
                message, {status}));
            const response = await app.inject({method: "POST", payload: input, url: "/scanner"});
            t.equal(response.statusCode, status);
            t.same(response.json(), {error: message});
        }
    }
});

tap.test(
    "rejects unexpected or malformed success and internal errors without leaking details",
    async (t) => {
        for (const upstream of [
            new Response("internal details", {status: constants.HTTP_STATUS_INTERNAL_SERVER_ERROR}),
            new Response("not JSON"),
            new Response(null, {status: 204}),
            new Response("redirect", {status: 302}),
            Response.json({}),
            Response.json({id: "1"}),
            Response.json({id: -1}),
            Response.json({id: 0}),
            Response.json({id: 1.5}),
            Response.json({id: Number.MAX_SAFE_INTEGER + 1}),
        ]) {
            const app = appFor(t, async () => upstream);
            const response = await app.inject({method: "POST", payload: input, url: "/scanner"});
            t.equal(response.statusCode, constants.HTTP_STATUS_BAD_GATEWAY);
            t.same(response.json(), {
                error: "Could not confirm scanner creation. Check the ingestor before retrying.",
            });
        }
    }
);

tap.test("uses the default HTTP transport against a real upstream", async (t) => {
    const upstream = createServer((_, response) => {
        response.setHeader("Content-Type", "application/json");
        response.end(JSON.stringify({id: 42}));
    });

    await new Promise<void>((resolve) => {
        upstream.listen(0, "127.0.0.1", resolve);
    });
    t.teardown(() => new Promise<void>((resolve, reject) => {
        upstream.close((error) => {
            if (error) {
                reject(error);
            } else {
                resolve();
            }
        });
    }));
    const address = upstream.address();
    if (null === address || "string" === typeof address) {
        throw new Error("Expected an upstream TCP address");
    }
    const configuration = enabledSettings();
    configuration.server.LogIngestorUrl = `http://127.0.0.1:${String(address.port)}`;
    const app = fastify();
    app.register(createS3ScannerRoutes(configuration), {prefix: "/scanner"});
    t.teardown(() => app.close());
    const response = await app.inject({method: "POST", payload: input, url: "/scanner"});
    t.equal(response.statusCode, constants.HTTP_STATUS_OK);
    t.same(response.json(), {id: 42});
});

tap.test("preserves other JSON rejection bodies as text", async (t) => {
    const app = appFor(t, async () => Response.json({message: "Invalid input"}, {
        status: constants.HTTP_STATUS_UNPROCESSABLE_ENTITY,
    }));
    const response = await app.inject({method: "POST", payload: input, url: "/scanner"});
    t.same(response.json(), {error: JSON.stringify({message: "Invalid input"})});
});

tap.test("does not retry an unavailable or timed-out creation request", async (t) => {
    let calls = 0;
    const app = appFor(t, async () => {
        calls++;
        throw new Error("private connection details");
    });

    const unavailable = await app.inject({method: "POST", payload: input, url: "/scanner"});
    t.equal(unavailable.statusCode, constants.HTTP_STATUS_BAD_GATEWAY);
    t.equal(calls, 1);
    let timedOut = false;
    const slow = appFor(t, async (_, options) => {
        calls++;

        return new Promise<Response>((resolve, reject) => {
            const timer = setTimeout(() => {
                resolve(Response.json({id: 1}));
            }, 100);

            options?.signal?.addEventListener("abort", () => {
                clearTimeout(timer);
                timedOut = true;
                reject(new Error("request timed out"));
            }, {once: true});
        });
    }, enabledSettings(), 10);

    const timedOutResponse = await slow.inject({method: "POST", payload: input, url: "/scanner"});
    t.equal(timedOutResponse.statusCode, constants.HTTP_STATUS_BAD_GATEWAY);
    t.ok(timedOut);
    t.equal(calls, 2);
});
