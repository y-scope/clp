import {
    type FastifyPluginAsyncTypebox,
    TypeBoxValidatorCompiler,
} from "@fastify/type-provider-typebox";
import {Value} from "@sinclair/typebox/value";
import {
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";
import {
    S3ScannerCreationSchema,
    S3ScannerErrorSchema,
    S3ScannerJobSchema,
    toS3ScannerConfig,
} from "@webui/common/schemas/ingestion";
import type {WebuiSettings} from "@webui/common/schemas/settings";
import {constants} from "http2";


const SERVICE_ERROR = "Could not confirm scanner creation. Check the ingestor before retrying.";
const DISABLED_ERROR = "S3 scanner ingestion is not enabled for this deployment.";
const MAX_BODY_BYTES = 16_384;

/**
 * Reads user-input errors from either native JSON errors or Axum's text rejections.
 *
 * @param body
 * @return The ingestor's user-facing error message.
 */
const getInputError = (body: string): string => {
    try {
        const parsed: unknown = JSON.parse(body);
        if (Value.Check(S3ScannerErrorSchema, parsed)) {
            return parsed.error;
        }
    } catch {
        // Axum may return plain text when request deserialization fails.
    }

    return body;
};

/**
 * Builds the scanner route with replaceable transport for deterministic boundary tests.
 *
 * @param configuration
 * @param send
 * @param timeoutMs
 * @return The scanner creation route.
 */
const createS3ScannerRoutes = (
    configuration: WebuiSettings,
    send: typeof fetch = fetch,
    timeoutMs = 10_000,
): FastifyPluginAsyncTypebox => async (fastify) => {
    // TypeBox validates without AJV's coercion/removal of unexpected fields.
    fastify.setValidatorCompiler(TypeBoxValidatorCompiler);
    fastify.post("/", {
        attachValidation: true,
        bodyLimit: MAX_BODY_BYTES,
        schema: {
            body: S3ScannerCreationSchema,
            response: {
                "200": S3ScannerJobSchema,
                "4xx": S3ScannerErrorSchema,
                "5xx": S3ScannerErrorSchema,
            },
        },
    }, async (request, reply) => {
        const ingestorUrl = configuration.server.LogIngestorUrl;
        if (!ingestorUrl ||
            CLP_STORAGE_ENGINES.CLP_S !== configuration.public.ClpStorageEngine ||
            STORAGE_TYPE.S3 !== configuration.public.LogsInputType) {
            return reply.code(constants.HTTP_STATUS_SERVICE_UNAVAILABLE).send({
                error: DISABLED_ERROR,
            });
        }
        if (request.validationError) {
            return reply.code(constants.HTTP_STATUS_BAD_REQUEST)
                .send({error: request.validationError.message});
        }

        try {
            const response = await send(new URL("/s3_scanner", ingestorUrl), {
                body: JSON.stringify(toS3ScannerConfig(request.body)),
                headers: {"Content-Type": "application/json"},
                method: "POST",
                redirect: "error",
                signal: AbortSignal.timeout(timeoutMs),
            });
            const body = await response.text();
            if (constants.HTTP_STATUS_BAD_REQUEST <= response.status &&
                constants.HTTP_STATUS_INTERNAL_SERVER_ERROR > response.status) {
                return await reply.code(response.status).send({error: getInputError(body)});
            }
            if (constants.HTTP_STATUS_OK !== response.status) {
                throw new Error(`Unexpected scanner creation status: ${String(response.status)}`);
            }
            const result: unknown = JSON.parse(body);
            if (!Value.Check(S3ScannerJobSchema, result)) {
                throw new Error("Invalid scanner creation response");
            }

            return {id: result.id};
        } catch (error: unknown) {
            request.log.error(error, "Unable to confirm scanner creation with the log ingestor");

            return reply.code(constants.HTTP_STATUS_BAD_GATEWAY).send({
                error: SERVICE_ERROR,
            });
        }
    });
};


export {createS3ScannerRoutes};
