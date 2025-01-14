import {StatusCodes} from "http-status-codes";

import settings from "../../settings.json" with {type: "json"};
import {EXTRACT_JOB_TYPES} from "../DbManager.js";
import S3Manager from "../S3Manager.js";


/**
 * Creates query routes.
 *
 * @param {import("fastify").FastifyInstance | {dbManager: DbManager}} fastify
 * @param {import("fastify").FastifyPluginOptions} options
 * @return {Promise<void>}
 */
const routes = async (fastify, options) => {
    const s3Manager = (
        null === settings.StreamFilesS3PathPrefix ||
        0 === settings.StreamFilesS3Region.length
    ) ?
        null :
        new S3Manager(
            settings.StreamFilesS3Region,
            settings.StreamS3AccessKeyId,
            settings.StreamS3SecretAccessKey
        );

    fastify.post("/query/extract-stream", async (req, resp) => {
        const {extractJobType, logEventIdx, streamId} = req.body;
        if (false === EXTRACT_JOB_TYPES.includes(extractJobType)) {
            resp.code(StatusCodes.BAD_REQUEST);
            throw new Error(`Invalid extractJobType="${extractJobType}".`);
        }

        if ("string" !== typeof streamId || 0 === streamId.trim().length) {
            resp.code(StatusCodes.BAD_REQUEST);
            throw new Error("\"streamId\" must be a non-empty string.");
        }

        const sanitizedLogEventIdx = Number(logEventIdx);
        let streamMetadata = await fastify.dbManager.getExtractedStreamFileMetadata(
            streamId,
            sanitizedLogEventIdx
        );

        if (null === streamMetadata) {
            const extractResult = await fastify.dbManager.submitAndWaitForExtractStreamJob({
                jobType: extractJobType,
                logEventIdx: sanitizedLogEventIdx,
                streamId: streamId,
                targetUncompressedSize: settings.StreamTargetUncompressedSize,
            });

            if (null === extractResult) {
                resp.code(StatusCodes.BAD_REQUEST);
                throw new Error("Unable to extract stream with " +
                    `streamId=${streamId} at logEventIdx=${sanitizedLogEventIdx}`);
            }

            streamMetadata = await fastify.dbManager.getExtractedStreamFileMetadata(
                streamId,
                sanitizedLogEventIdx
            );

            if (null === streamMetadata) {
                resp.code(StatusCodes.BAD_REQUEST);
                throw new Error("Unable to find the metadata of extracted stream with " +
                    `streamId=${streamId} at logEventIdx=${sanitizedLogEventIdx}`);
            }
        }

        if (null === s3Manager) {
            streamMetadata.path = `/streams/${streamMetadata.path}`;
        } else {
            streamMetadata.path = await s3Manager.getPreSignedUrl(
                `s3://${settings.StreamFilesS3PathPrefix}${streamMetadata.path}`
            );
        }

        return streamMetadata;
    });
};

export default routes;
