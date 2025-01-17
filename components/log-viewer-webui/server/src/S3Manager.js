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
     */
    constructor (
        region,
    ) {
        this.#s3Client = new S3Client({
            region: region,
        });
    }

    /**
     * Generates a pre-signed URL for accessing an S3 object.
     *
     * @param {string} s3uriString The S3 object URI string.
     * @return {Promise<string>} The pre-signed URL string.
     */
    async getPreSignedUrl (s3uriString) {
        const s3uri = new URL(s3uriString);
        const command = new GetObjectCommand({
            Bucket: s3uri.hostname,
            Key: s3uri.pathname.substring(1),
        });

        return await getSignedUrl(
            this.#s3Client,
            command,
            {
                expiresIn: PRE_SIGNED_URL_EXPIRY_TIME_SECONDS,
            }
        );
    }
}

export default S3Manager;
