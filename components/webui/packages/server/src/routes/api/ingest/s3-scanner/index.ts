import {
    type FastifyPluginAsyncTypebox,
    TypeBoxValidatorCompiler,
} from "@fastify/type-provider-typebox";
import {Value} from "@sinclair/typebox/value";
import {
    CLP_DEFAULT_DATASET_NAME,
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";
import {ErrorSchema} from "@webui/common/schemas/error";
import {
    type S3ScannerCreation,
    S3ScannerCreationSchema,
    S3ScannerJobSchema,
} from "@webui/common/schemas/ingestion";
import {constants} from "http2";

import {
    publicSettings,
    serverSettings,
} from "../../../../settings.js";


const SERVICE_ERROR = "Could not confirm scanner creation. Check the ingestor before retrying.";
const DISABLED_ERROR = "S3 scanner ingestion is not enabled for this deployment.";
const MAX_BODY_BYTES = 16_384;
const REQUEST_TIMEOUT_MS = 10_000;

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
 * Maps allowed form values to the existing log-ingestor request contract.
 *
 * @param input
 * @return A scanner config using native defaults for omitted tuning fields.
 */
const toS3ScannerConfig = (input: S3ScannerCreation) => ({
    bucket_name: input.bucket,
    dataset: input.dataset || CLP_DEFAULT_DATASET_NAME,
    key_prefix: input.keyPrefix,
    region: input.region,
    ...(!input.unstructured && input.timestampKey ?
        {timestamp_key: input.timestampKey} :
        {}),
    unstructured: input.unstructured ?? false,
});

/**
 * Scanner creation API route.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    // TypeBox validates without AJV's coercion/removal of unexpected fields.
    fastify.setValidatorCompiler(TypeBoxValidatorCompiler);
    fastify.post("/", {
        attachValidation: true,
        bodyLimit: MAX_BODY_BYTES,
        schema: {
            body: S3ScannerCreationSchema,
            response: {
                "200": S3ScannerJobSchema,
                "4xx": ErrorSchema,
                "5xx": ErrorSchema,
            },
        },
    }, async (request, reply) => {
        const ingestorUrl = serverSettings.LogIngestorUrl;
        if (!ingestorUrl ||
            CLP_STORAGE_ENGINES.CLP_S !== publicSettings.ClpStorageEngine ||
            STORAGE_TYPE.S3 !== publicSettings.LogsInputType) {
            return reply.code(constants.HTTP_STATUS_SERVICE_UNAVAILABLE).send({
                message: DISABLED_ERROR,
            });
        }
        if (request.validationError) {
            return reply.code(constants.HTTP_STATUS_BAD_REQUEST)
                .send({message: request.validationError.message});
        }

        try {
            const response = await fetch(new URL("/s3_scanner", ingestorUrl), {
                body: JSON.stringify(toS3ScannerConfig(request.body)),
                headers: {"Content-Type": "application/json"},
                method: "POST",
                redirect: "error",
                signal: AbortSignal.timeout(REQUEST_TIMEOUT_MS),
            });
            const body = await response.text();
            if (constants.HTTP_STATUS_BAD_REQUEST <= response.status &&
                constants.HTTP_STATUS_INTERNAL_SERVER_ERROR > response.status) {
                return await reply.code(response.status).send({message: getInputError(body)});
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
                message: SERVICE_ERROR,
            });
        }
    });
};


export default plugin;
