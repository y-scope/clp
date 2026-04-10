import {
    GetObjectCommand,
    S3Client,
    S3ClientConfig,
} from "@aws-sdk/client-s3";
import {getSignedUrl} from "@aws-sdk/s3-request-presigner";
import {Nullable} from "@webui/common/utility-types";
import fp from "fastify-plugin";

import settings from "../../../../settings.json" with {type: "json"};
import {PRE_SIGNED_URL_EXPIRY_TIME_SECONDS} from "./typings.js";


/**
 * Class to manage Simple Storage Service (S3) objects for stream files.
 */
class S3Manager {
    #s3Client: Nullable<S3Client> = null;

    readonly #s3ClientConfig: S3ClientConfig;

    /**
     * @param config
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
}

declare module "fastify" {
    interface FastifyInstance {
        StreamFilesS3Manager?: S3Manager;
    }
}

export default fp(
    (fastify) => {
        const region = settings.StreamFilesS3Region;
        const profile = settings.StreamFilesS3Profile;

        // Only decorate if the region is set (i.e. s3 support is configured in package)
        // Disable no-unnecessary-condition since linter doesn't understand that settings
        // values are not hardcoded.
        // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
        if (null !== region && "" !== region) {
            const {
                CLP_STREAM_OUTPUT_AWS_ACCESS_KEY_ID: accessKeyId,
                CLP_STREAM_OUTPUT_AWS_SECRET_ACCESS_KEY: secretAccessKey,
            } = fastify.config;

            const s3ClientConfig: S3ClientConfig = {
                ...((accessKeyId && secretAccessKey) ?
                    {credentials: {accessKeyId, secretAccessKey}} :
                    {}),
                // eslint-disable-next-line @typescript-eslint/no-unnecessary-condition
                ...((null !== profile) && {profile}),
                region,
            };

            fastify.log.info(
                {region, profile},
                "Initializing StreamFilesS3Manager"
            );
            fastify.decorate("StreamFilesS3Manager", new S3Manager(s3ClientConfig));
        }
    },
);
