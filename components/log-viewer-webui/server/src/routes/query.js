// eslint-disable-next-line no-magic-numbers
const EXTRACT_IR_TARGET_UNCOMPRESSED_SIZE = 128 * 1024 * 1024;

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
            const extractResult = await fastify.dbManager.submitAndWaitForExtractIrJob({
                file_split_id: null,
                msg_ix: sanitizedLogEventIdx,
                orig_file_id: origFileId,
                target_uncompressed_size: EXTRACT_IR_TARGET_UNCOMPRESSED_SIZE,
            });

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
};

export default routes;
