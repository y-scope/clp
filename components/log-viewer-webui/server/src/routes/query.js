// eslint-disable-next-line no-magic-numbers
const EXTRACT_IR_TARGET_UNCOMPRESSED_SIZE = 128 * 1024 * 1024;
const EXTRACT_JSON_TARGET_CHUNK_SIZE = 100000;

import {QUERY_JOB_TYPE} from "../DbManager.js";


/**
 * Creates query routes.
 *
 * @param {import("fastify").FastifyInstance | {dbManager: DbManager}} fastify
 * @param {import("fastify").FastifyPluginOptions} options
 * @return {Promise<void>}
 */
const routes = async (fastify, options) => {
    fastify.post("/query/extract-stream", async (req, resp) => {
        const {queryJobType, targetId, logEventIdx} = req.body;
        const sanitizedLogEventIdx = Number(logEventIdx);

        let streamMetadata = await fastify.dbManager.getExtractedStreamFileMetadata(
            targetId,
            sanitizedLogEventIdx
        );

        if (null === streamMetadata) {
            let jobConfig;
            if (QUERY_JOB_TYPE.EXTRACT_IR === queryJobType) {
                jobConfig = {
                    file_split_id: null,
                    msg_ix: sanitizedLogEventIdx,
                    orig_file_id: targetId,
                    target_uncompressed_size: EXTRACT_IR_TARGET_UNCOMPRESSED_SIZE,
                };
            } else if (QUERY_JOB_TYPE.EXTRACT_JSON === queryJobType) {
                jobConfig = {
                    archive_id: targetId,
                    target_chunk_size: EXTRACT_JSON_TARGET_CHUNK_SIZE,
                };
            } else {
                const err = new Error(`Unsupported Job type: ${queryJobType});`);
                err.statusCode = 400;
                throw err;
            }
            const extractResult = await fastify.dbManager.submitAndWaitForExtractStreamJob(
                jobConfig,
                queryJobType
            );

            if (null === extractResult) {
                const err = new Error("Unable to extract json for file with " +
                    `archiveId=${archiveId} at timestamp=${sanitizedTimestamp}`);

                err.statusCode = 400;
                throw err;
            }
            streamMetadata = await fastify.dbManager.getExtractedStreamFileMetadata(
                targetId,
                logEventIdx
            );
        }

        return streamMetadata;
    });
};

export default routes;
