import {
    ListObjectsV2Command,
    S3Client,
} from "@aws-sdk/client-s3";
import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {ErrorSchema} from "@webui/common/schemas/error";
import {
    S3Entry,
    S3ListRequest,
    S3ListRequestSchema,
    S3ListResponse,
    S3ListResponseSchema,
} from "@webui/common/schemas/s3";
import {constants} from "http2";

import settings from "../../../../settings.json" with {type: "json"};


/**
 * Default delimiter for S3 listing to emulate a folder hierarchy.
 */
const S3_DELIMITER = "/";

/**
 * Maximum number of keys to return per S3 listing request.
 */
const S3_MAX_KEYS = 200;

/**
 * Creates an S3Client using credentials from server settings and per-request overrides.
 *
 * @param regionCode
 * @param endpointUrl
 * @return A configured S3Client instance.
 */
const createS3Client = (regionCode?: string, endpointUrl?: string): S3Client => {
    const awsAuth = settings.LogsInputS3AwsAuthentication as {
        type: string;
        credentials?: {
            access_key_id: string;
            secret_access_key: string;
            session_token?: string | null;
        } | null;
    } | null;

    const clientConfig: ConstructorParameters<typeof S3Client>[0] = {};
    if (regionCode) {
        clientConfig.region = regionCode;
    }
    if (endpointUrl) {
        clientConfig.endpoint = endpointUrl;
        clientConfig.forcePathStyle = true;
    }
    if (awsAuth?.credentials) {
        clientConfig.credentials = {
            accessKeyId: awsAuth.credentials.access_key_id,
            secretAccessKey: awsAuth.credentials.secret_access_key,
            ...(awsAuth.credentials.session_token && {
                sessionToken: awsAuth.credentials.session_token,
            }),
        };
    }

    return new S3Client(clientConfig);
};

/**
 * Lists S3 objects and common prefixes under the given bucket/prefix.
 *
 * @param params
 * @return The listing response.
 */
const listObjects = async (params: S3ListRequest): Promise<S3ListResponse> => {
    const s3Client = createS3Client(params.regionCode, params.endpointUrl);
    try {
        const command = new ListObjectsV2Command({
            Bucket: params.bucket,
            ContinuationToken: params.continuationToken,
            Delimiter: S3_DELIMITER,
            MaxKeys: S3_MAX_KEYS,
            Prefix: params.prefix ?? "",
        });

        const response = await s3Client.send(command);

        const entries: S3Entry[] = [
            ...(response.CommonPrefixes ?? [])
                .filter((cp) => "string" === typeof cp.Prefix)
                .map((cp) => ({isPrefix: true, key: cp.Prefix as string})),
            ...(response.Contents ?? [])
                .filter((obj) => "string" === typeof obj.Key && obj.Key !== params.prefix)
                .map((obj) => ({isPrefix: false, key: obj.Key as string})),
        ];

        return {
            entries: entries,
            isTruncated: response.IsTruncated ?? false,
            nextContinuationToken: response.NextContinuationToken ?? null,
        };
    } finally {
        s3Client.destroy();
    }
};

/**
 * S3 listing API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    fastify.get(
        "/ls",
        {
            schema: {
                querystring: S3ListRequestSchema,
                response: {
                    [constants.HTTP_STATUS_OK]: S3ListResponseSchema,
                    [constants.HTTP_STATUS_INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["S3"],
            },
        },
        async (request, reply) => {
            try {
                return await listObjects(request.query);
            } catch (err: unknown) {
                const errMsg = err instanceof Error ?
                    err.message :
                    "Failed to list S3 objects";

                request.log.error(err, "S3 listing failed");

                return reply.internalServerError(errMsg);
            }
        }
    );
};

export default plugin;
