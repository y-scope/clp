import type {Readable} from "node:stream";
import {text} from "node:stream/consumers";

import fastifyHttpProxy, {type FastifyHttpProxyOptions} from "@fastify/http-proxy";
import {
    type FastifyPluginAsyncTypebox,
    TypeBoxValidatorCompiler,
} from "@fastify/type-provider-typebox";
import {Value} from "@sinclair/typebox/value";
import {
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";
import {ErrorSchema} from "@webui/common/schemas/error";
import {
    S3ScannerConfigSchema,
    S3ScannerJobSchema,
} from "@webui/common/schemas/ingestion";
import type {
    FastifyError,
    FastifyReply,
} from "fastify";
import {constants} from "http2";

import {
    publicSettings,
    serverSettings,
} from "../../settings.js";


const SERVICE_ERROR = "Could not confirm scanner creation. Check the ingestor before retrying.";
const MAX_BODY_BYTES = 16_384;
const REQUEST_TIMEOUT_MS = 10_000;

type ProxyHandler = NonNullable<FastifyHttpProxyOptions["handler"]>;

/**
 * Reads user-input errors from either native JSON errors or Axum's text rejections.
 *
 * @param body
 * @return The ingestor's user-facing error message.
 */
const getInputError = (body: string): string => {
    try {
        const parsed: unknown = JSON.parse(body);
        if (null !== parsed && "object" === typeof parsed && "error" in parsed &&
            "string" === typeof parsed.error) {
            return parsed.error;
        }
    } catch {
        // Axum may return plain text when request deserialization fails.
    }

    return body;
};

/**
 * Logs transport/response errors without exposing upstream details to the browser.
 *
 * @param reply
 * @param error
 */
const sendServiceError = (reply: FastifyReply, error: unknown): void => {
    reply.log.error(error, "Unable to confirm scanner creation with the log ingestor");
    if (false === reply.sent) {
        reply.code(constants.HTTP_STATUS_BAD_GATEWAY).send({message: SERVICE_ERROR});
    }
};

/**
 * Reads and validates the upstream response before sending any of its contents.
 *
 * @param reply
 * @param stream
 * @param status
 */
const sendValidatedResponse = async (
    reply: FastifyReply,
    stream: Readable,
    status: number,
): Promise<void> => {
    const body = await text(stream);
    if (reply.sent) {
        return;
    }
    if (constants.HTTP_STATUS_BAD_REQUEST <= status &&
        constants.HTTP_STATUS_INTERNAL_SERVER_ERROR > status) {
        reply.code(status).send({message: getInputError(body)});

        return;
    }
    if (constants.HTTP_STATUS_OK !== status) {
        throw new Error(`Unexpected scanner creation status: ${String(status)}`);
    }
    const result: unknown = JSON.parse(body);
    if (false === Value.Check(S3ScannerJobSchema, result)) {
        throw new Error("Invalid scanner creation response");
    }
    reply.send({id: result.id});
};

/**
 * Forwards a validated request with a deadline covering headers and the complete response body.
 *
 * @param _request
 * @param reply
 * @param destination
 * @param options
 * @return The pending proxy reply.
 */
const forwardScanner: ProxyHandler = (_request, reply, destination, options) => {
    let upstreamStream: Readable | null = null;
    const timer = setTimeout(() => {
        const error = new Error("Scanner creation request timed out");
        upstreamStream?.destroy(error);
        sendServiceError(reply, error);
    }, REQUEST_TIMEOUT_MS);

    timer.unref();
    reply.raw.once("close", () => {
        clearTimeout(timer);
        upstreamStream?.destroy();
    });

    return reply.from(destination, {
        ...options,
        onError: (_, {error}) => {
            clearTimeout(timer);
            sendServiceError(reply, error);
        },
        onResponse: (_, __, response) => {
            upstreamStream = response.stream;
            if (reply.sent) {
                upstreamStream.destroy();

                return;
            }
            sendValidatedResponse(reply, upstreamStream, response.statusCode)
                .catch((error: unknown) => {
                    sendServiceError(reply, error);
                })
                .finally(() => {
                    clearTimeout(timer);
                });
        },
        queryString: () => "",
        retriesCount: 0,

        // Responses are validated and reserialized; upstream headers no longer apply.
        rewriteHeaders: () => ({}),
        rewriteRequestHeaders: (_, headers) => ({
            "content-length": headers["content-length"],
            "content-type": "application/json",
        }),
        timeout: REQUEST_TIMEOUT_MS,
    });
};

/**
 * Proxies only scanner creation, validating both sides of the native API boundary.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const ingestorUrl = serverSettings.LogIngestorUrl;
    if (null === ingestorUrl || "" === ingestorUrl ||
        CLP_STORAGE_ENGINES.CLP_S !== publicSettings.ClpStorageEngine ||
        STORAGE_TYPE.S3 !== publicSettings.LogsInputType) {
        return;
    }

    fastify.setErrorHandler<FastifyError>((error, _, reply) => {
        const status = error.statusCode;
        if ("number" === typeof status && constants.HTTP_STATUS_BAD_REQUEST <= status &&
            constants.HTTP_STATUS_INTERNAL_SERVER_ERROR > status) {
            reply.code(status).send({message: error.message});
        } else {
            sendServiceError(reply, error);
        }
    });
    fastify.setValidatorCompiler(TypeBoxValidatorCompiler);
    fastify.addHook("onRoute", (route) => {
        route.bodyLimit = MAX_BODY_BYTES;
        route.schema = {
            body: S3ScannerConfigSchema,
            response: {
                "200": S3ScannerJobSchema,
                "4xx": ErrorSchema,
                "5xx": ErrorSchema,
            },
        };
    });
    await fastify.register(fastifyHttpProxy, {
        destroyAgent: true,
        handler: forwardScanner,
        httpMethods: ["POST"],
        prefix: "/api/log-ingestor",
        proxyPayloads: false,
        retryMethods: [],
        rewritePrefix: "",
        routes: ["/s3_scanner"],
        undici: {
            bodyTimeout: REQUEST_TIMEOUT_MS,
            connectTimeout: REQUEST_TIMEOUT_MS,
            headersTimeout: REQUEST_TIMEOUT_MS,
        },
        upstream: ingestorUrl,
    });
};


export default plugin;
