import {
    GetObjectCommand,
    S3Client,
} from "@aws-sdk/client-s3";
import {getSignedUrl} from "@aws-sdk/s3-request-presigner";
import fastifyPlugin from "fastify-plugin";

import {Nullable} from "../typings/common.js";


/**
 * Expiry time in seconds for pre-signed URLs.
 */
const PRE_SIGNED_URL_EXPIRY_TIME_SECONDS = 3600;

/**
 * Class to manage Simple Storage Service (S3) objects.
 */
class S3Manager {
    readonly #s3Client;

    /**
     * @param region
     * @param [profile]
     */
    constructor (region: string, profile: Nullable<string>) {
        this.#s3Client = new S3Client({
            region,
            ...((null !== profile) && {profile}),
        });
    }

    /**
     * Generates a pre-signed URL for accessing an S3 object.
     *
     * @param s3UriString The S3 object URI string.
     * @return The pre-signed URL string.
     * @throws {Error} If a pre-signed URL couldn't be generated.
     */
    async getPreSignedUrl (s3UriString: string): Promise<string> {
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

/**
 * Initializes a Fastify plugin, which decorates the application with an S3 manager at the
 * "s3Manager" property only when all plugin options are valid.
 */
export default fastifyPlugin(
    async (app, options: {region: Nullable<string>; profile: Nullable<string>}) => {
        const {region, profile} = options;
        if (null === region) {
            return;
        }

        console.log(`Initializing S3Manager with region="${region}" and profile="${profile}"...`);
        app.decorate("s3Manager", new S3Manager(region, profile));
    }
);

declare module "fastify" {
    interface FastifyInstance {
        s3Manager?: S3Manager;
    }
}
