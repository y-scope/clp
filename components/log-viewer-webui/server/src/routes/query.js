import settings from "../../settings.json" with {type: "json"};
import {QUERY_JOB_TYPE} from "../DbManager.js";


/**
 * Submits a stream extraction job with the given parameters and waits for it.
 *
 * @param {import("fastify").FastifyInstance | {dbManager: DbManager}} fastify
 * @param {QUERY_JOB_TYPE} extractJobType
 * @param {string} streamId
 * @param {int} sanitizedLogEventIdx
 * @throws {Error} if the extract stream job submission fails.
 */
const submitAndWaitForExtractStreamJob = async (
    fastify,
    extractJobType,
    streamId,
    sanitizedLogEventIdx
) => {
    let jobConfig;
    const streamTargetUncompressedSize = settings.StreamTargetUncompressedSize;
    if (QUERY_JOB_TYPE.EXTRACT_IR === extractJobType) {
        jobConfig = {
            file_split_id: null,
            msg_ix: sanitizedLogEventIdx,
            orig_file_id: streamId,
            target_uncompressed_size: streamTargetUncompressedSize,
        };
    } else if (QUERY_JOB_TYPE.EXTRACT_JSON === extractJobType) {
        jobConfig = {
            archive_id: streamId,
            target_chunk_size: streamTargetUncompressedSize,
        };
    }

    if (null === jobConfig) {
        const err = new Error(`Unsupported Job type: ${extractJobType}`);
        err.statusCode = 400;
        throw err;
    }

    const extractResult = await fastify.dbManager.submitAndWaitForExtractStreamJob(
        jobConfig,
        extractJobType
    );

    if (null === extractResult) {
        const err = new Error("Unable to extract stream with " +
            `streamId=${streamId} at logEventIdx=${sanitizedLogEventIdx}`);

        err.statusCode = 400;
        throw err;
    }
};

/**
 * Creates query routes.
 *
 * @param {import("fastify").FastifyInstance | {dbManager: DbManager}} fastify
 * @param {import("fastify").FastifyPluginOptions} options
 * @return {Promise<void>}
 */
const routes = async (fastify, options) => {
    fastify.post("/query/extract-stream", async (req, resp) => {
        const {extractJobType, logEventIdx, streamId} = req.body;
        const sanitizedLogEventIdx = Number(logEventIdx);
        let streamMetadata = await fastify.dbManager.getExtractedStreamFileMetadata(
            streamId,
            sanitizedLogEventIdx
        );

        if (null === streamMetadata) {
            await submitAndWaitForExtractStreamJob(
                fastify,
                extractJobType,
                streamId,
                sanitizedLogEventIdx
            );
            streamMetadata = await fastify.dbManager.getExtractedStreamFileMetadata(
                streamId,
                sanitizedLogEventIdx
            );

            if (null === streamMetadata) {
                const err = new Error("Unable to find the metadata of extracted stream with " +
                    `streamId=${streamId} at logEventIdx=${sanitizedLogEventIdx}`);

                err.statusCode = 400;
                throw err;
            }
        }

        return streamMetadata;
    });
};

export default routes;
