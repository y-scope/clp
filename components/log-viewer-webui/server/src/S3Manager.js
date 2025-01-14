import {
    GetObjectCommand,
    S3Client,
} from "@aws-sdk/client-s3";
import {getSignedUrl} from "@aws-sdk/s3-request-presigner";


/**
 * Class for validating s3 inputs.
 */
class S3Manager {
    #s3Client;

    /**
     * TODO: need to handle when access_key = None.
     *
     * @param {string} region
     * @param {string} accessKeyId
     * @param {string} secretAccessKey
     */
    constructor (
        region,
        accessKeyId,
        secretAccessKey
    ) {
        this.#s3Client = new S3Client({
            region: region,
            credentials: {
                accessKeyId: accessKeyId,
                secretAccessKey: secretAccessKey,
            },
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

        return await getSignedUrl(this.#s3Client, command, {expiresIn: 3600});
    }
}

export default S3Manager;
