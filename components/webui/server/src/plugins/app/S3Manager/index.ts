import {
    GetObjectCommand,
    ListObjectsV2Command,
    S3Client,
    S3ClientConfig,
} from "@aws-sdk/client-s3";
import {fromIni} from "@aws-sdk/credential-providers";
import {getSignedUrl} from "@aws-sdk/s3-request-presigner";
import {
    S3Entry,
    S3ListResponse,
} from "@webui/common/schemas/s3";
import {Nullable} from "@webui/common/utility-types";
import fp from "fastify-plugin";

import settings from "../../../../settings.json" with {type: "json"};
import {PRE_SIGNED_URL_EXPIRY_TIME_SECONDS} from "./typings.js";


/**
 * Default delimiter for S3 listing to emulate a folder hierarchy.
 */
const S3_DELIMITER = "/";


/**
 * Resolves AWS credentials from access keys or a named profile.
 *
 * Priority: access keys > named profile > SDK default credential chain (IRSA, etc.).
 *
 * @param accessKeyId AWS access key ID, or empty string if not set.
 * @param secretAccessKey AWS secret access key, or empty string if not set.
 * @param profile Named profile for `fromIni`, or null.
 * @return Partial `S3ClientConfig` with credentials if resolved, or empty for default chain.
 */
const resolveAwsCredentials = (
    accessKeyId: Nullable<string>,
    secretAccessKey: Nullable<string>,
    profile: Nullable<string>,
): S3ClientConfig => {
    if (null !== accessKeyId && null !== secretAccessKey) {
        return {
            credentials: {
                accessKeyId,
                secretAccessKey,
            },
        };
    }
    if (null !== profile) {
        return {credentials: fromIni({profile})};
    }

    return {};
};

/**
 * Class to manage Simple Storage Service (S3) objects for stream files.
 */
class S3Manager {
    #s3Client: Nullable<S3Client> = null;

    readonly #s3ClientConfig: S3ClientConfig;

    /**
     * @param config S3 client configuration.
     */
    constructor (config: S3ClientConfig) {
        this.#s3ClientConfig = config;
    }

    /**
     * Generates a pre-signed URL for accessing an S3 object.
     *
     * @param s3UriString The S3 object URI string.
     * @return The pre-signed URL string.
     * @throws {Error} If a pre-signed URL couldn't be generated.
     */
    async getPreSignedUrl (s3UriString: string): Promise<string> {
        if (null === this.#s3Client) {
            this.#s3Client = new S3Client(this.#s3ClientConfig);
        }

        const s3Uri = new URL(s3UriString);
        const command = new GetObjectCommand({
            Bucket: s3Uri.hostname,
            Key: s3Uri.pathname.substring(1),
        });

        try {
            return await getSignedUrl(
                this.#s3Client,
                command,
                {
                    expiresIn: PRE_SIGNED_URL_EXPIRY_TIME_SECONDS,
                }
            );
        } catch (error: unknown) {
            if (false === error instanceof Error) {
                throw error;
            }
            throw new Error(`Failed to generate pre-signed URL: ${error.message}`);
        }
    }

    /**
     * Lists S3 objects and common prefixes under the given bucket/prefix.
     *
     * @param bucket S3 bucket name.
     * @param region AWS region for the bucket.
     * @param prefix S3 key prefix to list under.
     * @param continuationToken Optional pagination token.
     * @return The listing response.
     */
    async listObjects (
        bucket: string,
        region: string,
        prefix: string,
        continuationToken?: string,
    ): Promise<S3ListResponse> {
        if (null === this.#s3Client || this.#s3Client.config.region !== region) {
            this.#s3Client = new S3Client({
                region,
                ...this.#s3ClientConfig,
            });
        }

        const command = new ListObjectsV2Command({
            Bucket: bucket,
            ContinuationToken: continuationToken,
            Delimiter: S3_DELIMITER,
            Prefix: prefix,
        });

        const response = await this.#s3Client.send(command);
        const entries: S3Entry[] = [
            ...(response.CommonPrefixes ?? [])
                .filter((cp) => "string" === typeof cp.Prefix)
                .map((cp) => ({isPrefix: true, key: cp.Prefix as string})),
            ...(response.Contents ?? [])
                .filter((obj) => "string" === typeof obj.Key && obj.Key !== prefix)
                .map((obj) => ({isPrefix: false, key: obj.Key as string})),
        ];

        return {
            entries: entries,
            isTruncated: response.IsTruncated ?? false,
            nextContinuationToken: response.NextContinuationToken ?? null,
        };
    }
}

declare module "fastify" {
    interface FastifyInstance {
        LogsInputS3Manager?: S3Manager;
        StreamFilesS3Manager?: S3Manager;
    }
}

export default fp(
    (fastify) => {
        const {
            CLP_LOGS_INPUT_AWS_ACCESS_KEY_ID,
            CLP_LOGS_INPUT_AWS_SECRET_ACCESS_KEY,
            CLP_STREAM_OUTPUT_AWS_ACCESS_KEY_ID,
            CLP_STREAM_OUTPUT_AWS_SECRET_ACCESS_KEY,
        } = fastify.config;

        const logsInputAuthType = settings.LogsInputS3AwsAuthType as string | null;
        if (null === logsInputAuthType) {
            fastify.log.debug("S3 logs input not configured, skipping LogsInputS3Manager");
        } else {
            fastify.log.info("Initializing LogsInputS3Manager...");
            fastify.decorate(
                "LogsInputS3Manager",
                new S3Manager(resolveAwsCredentials(
                    CLP_LOGS_INPUT_AWS_ACCESS_KEY_ID,
                    CLP_LOGS_INPUT_AWS_SECRET_ACCESS_KEY,
                    settings.LogsInputS3AwsProfile
                )),
            );
        }

        const streamFilesRegion = settings.StreamFilesS3Region as string | null;
        if (null === streamFilesRegion || "" === streamFilesRegion) {
            fastify.log.debug("S3 stream files not configured, skipping StreamFilesS3Manager");
        } else {
            fastify.log.info("Initializing StreamFilesS3Manager...");
            fastify.decorate(
                "StreamFilesS3Manager",
                new S3Manager({
                    region: streamFilesRegion,
                    ...resolveAwsCredentials(
                        CLP_STREAM_OUTPUT_AWS_ACCESS_KEY_ID,
                        CLP_STREAM_OUTPUT_AWS_SECRET_ACCESS_KEY,
                        settings.StreamFilesS3Profile
                    ),
                }),
            );
        }
    },
);
