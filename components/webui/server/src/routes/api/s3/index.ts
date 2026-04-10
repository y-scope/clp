// eslint-disable-next-line @stylistic/max-len
import partitions from "@aws-sdk/util-endpoints/dist-cjs/lib/aws/partitions.json" with {type: "json"};
import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {ErrorSchema} from "@webui/common/schemas/error";
import {
    S3ListRequestSchema,
    S3ListResponseSchema,
} from "@webui/common/schemas/s3";
import {constants} from "http2";


/**
 * Valid AWS region codes from the standard (`aws`) partition.
 */
const awsPartition = partitions.partitions.find((p) => "aws" === p.id);
const validRegionCodes: Set<string> = new Set(
    awsPartition ?
        Object.keys(awsPartition.regions) :
        [],
);

/**
 * Maps AWS S3 error names to user-friendly messages.
 */
const S3_ERROR_MESSAGES: Record<string, string> = {
    AccessDenied: "Access denied. Check your AWS credentials and bucket permissions.",
    AuthorizationHeaderMalformed: "The AWS authorization header is malformed.",
    BucketAlreadyExists: "The requested bucket name already exists.",
    IllegalLocationConstraintException: "The bucket is in a different region than specified.",
    InvalidAccessKeyId: "The AWS access key ID does not exist.",
    InvalidBucketName: "The bucket name is invalid.",
    NoSuchBucket: "The specified bucket does not exist.",
    SignatureDoesNotMatch: "AWS signature does not match. Check your credentials.",
};

/**
 * Returns a user-friendly message for a known AWS S3 error, or a fallback.
 *
 * @param err
 * @param fallback
 * @return
 */
const getS3ErrorMessage = (err: unknown, fallback: string): string => {
    if (err instanceof Error && err.name in S3_ERROR_MESSAGES) {
        return S3_ERROR_MESSAGES[err.name] as string;
    }

    return fallback;
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
                    [constants.HTTP_STATUS_BAD_REQUEST]: ErrorSchema,
                    [constants.HTTP_STATUS_INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["S3"],
            },
        },
        async (request, reply) => {
            const logsInputS3Manager = fastify.LogsInputS3Manager;
            if ("undefined" === typeof logsInputS3Manager) {
                return reply.internalServerError(
                    "S3 logs input is not configured."
                );
            }

            const {bucket, continuationToken, prefix, region} = request.query;

            if (false === validRegionCodes.has(region)) {
                return reply.badRequest(`Unknown AWS region: "${region}".`);
            }

            try {
                return await logsInputS3Manager.listObjects(
                    bucket,
                    region,
                    prefix ?? "",
                    continuationToken,
                );
            } catch (err: unknown) {
                const errMsg = getS3ErrorMessage(err, "Failed to list S3 objects.");
                request.log.error(err, "S3 listing failed");

                reply.code(constants.HTTP_STATUS_INTERNAL_SERVER_ERROR);

                return {message: errMsg};
            }
        }
    );
};

export default plugin;
