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
    fastify.post("/query/extract-ir", async (req, resp) => {
        const {origFileId, logEventIdx} = req.body;
        const sanitizedLogEventIdx = Number(logEventIdx);

        let irMetadata = await fastify.dbManager.getExtractedIrFileMetadata(
            origFileId,
            sanitizedLogEventIdx
        );

        if (null === irMetadata) {
            const extractResult = await fastify.dbManager.submitAndWaitForExtractStreamJob(
                {
                    file_split_id: null,
                    msg_ix: sanitizedLogEventIdx,
                    orig_file_id: origFileId,
                    target_uncompressed_size: EXTRACT_IR_TARGET_UNCOMPRESSED_SIZE,
                },
                QUERY_JOB_TYPE.EXTRACT_IR
            );

            if (null === extractResult) {
                const err = new Error("Unable to extract IR for file with " +
                    `origFileId=${origFileId} at logEventIdx=${sanitizedLogEventIdx}`);

                err.statusCode = 400;
                throw err;
            }
            irMetadata = await fastify.dbManager.getExtractedIrFileMetadata(
                origFileId,
                sanitizedLogEventIdx
            );
        }

        return irMetadata;
    });
    fastify.post("/query/extract-json", async (req, resp) => {
        const {archiveId, timestamp} = req.body;
        const sanitizedTimestamp = Number(timestamp);

        let jsonMetadata = await fastify.dbManager.getExtractedJsonFileMetadata(
            archiveId,
            sanitizedTimestamp
        );

        if (null === jsonMetadata) {
            const extractResult = await fastify.dbManager.submitAndWaitForExtractStreamJob(
                {
                    archive_id: archiveId,
                    target_chunk_size: EXTRACT_JSON_TARGET_CHUNK_SIZE,
                },
                QUERY_JOB_TYPE.EXTRACT_JSON
            );

            if (null === extractResult) {
                const err = new Error("Unable to extract json for file with " +
                    `archiveId=${archiveId} at timestamp=${sanitizedTimestamp}`);

                err.statusCode = 400;
                throw err;
            }
            jsonMetadata = await fastify.dbManager.getExtractedJsonFileMetadata(
                archiveId,
                sanitizedTimestamp
            );
        }

        return jsonMetadata;
    });
};

export default routes;
