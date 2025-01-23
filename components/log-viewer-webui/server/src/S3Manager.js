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
    #s3Client = null;

    /**
     * @param {object} props
     * @param {string | null} props.region
     */
    constructor (props) {
        const region = props.region;
        if (region !== null) {
            this.#s3Client = new S3Client({
                region: region,
            });
        }
    }

    isEnabled () {
        return null !== this.#s3Client;
    }

    /**
     * Generates a pre-signed URL for accessing an S3 object.
     *
     * @param {string} s3UriString The S3 object URI string.
     * @return {Promise<string>} The pre-signed URL string.
     * @throws {Error} If a pre-signed URL couldn't be generated.
     */
    async getPreSignedUrl (s3UriString) {
        if (false === this.isEnabled()) {
            throw new Error(`pre-signed URL cannot be generated when S3Manager is not enabled`);
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
        } catch (error) {
            throw new Error(`Failed to generate pre-signed URL: ${error.message}`);
        }
    }
}

export default fastifyPlugin(async (app, options) => {
    await app.decorate("s3Manager", new S3Manager(options));
});
