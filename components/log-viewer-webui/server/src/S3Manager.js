import fastifyPlugin from "fastify-plugin";

import {
    GetObjectCommand,
    S3Client,
} from "@aws-sdk/client-s3";
import {getSignedUrl} from "@aws-sdk/s3-request-presigner";


/**
 * Expiry time in seconds for pre-signed URLs.
 */
const PRE_SIGNED_URL_EXPIRY_TIME_SECONDS = 3600;

/**
 * Class to manage Simple Storage Service (S3) objects.
 */
class S3Manager {
    #s3Client;

    /**
     * @param {string} region
     * @param {string} [profile]
     */
    constructor (region, profile) {
        if (profile) {
            this.#s3Client = new S3Client({
                region: region,
                profile: profile,
            });
        } else {
            this.#s3Client = new S3Client({
                region: region,
            });
        }
    }

    /**
     * Generates a pre-signed URL for accessing an S3 object.
     *
     * @param {string} s3UriString The S3 object URI string.
     * @return {Promise<string>} The pre-signed URL string.
     * @throws {Error} If a pre-signed URL couldn't be generated.
     */
    async getPreSignedUrl (s3UriString) {
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
        } catch (error) {
            throw new Error(`Failed to generate pre-signed URL: ${error.message}`);
        }
    }
}

/**
 * Initializes a Fastify plugin, which decorates the application with an S3 manager at the
 * "s3Manager" property when all plugin options are valid.
 */
export default fastifyPlugin(async (app, options) => {
    const {region, profile} = options;
    if (!region) {
        return;
    }

    console.log(`Initializing S3Manager with region="${region}" and profile="${profile}"...`);
    await app.decorate("s3Manager", new S3Manager(region, profile));
});
