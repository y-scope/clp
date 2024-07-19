/**
 * Creates query routes.
 *
 * @param {import("fastify").FastifyInstance | {dbManager: DbManager}} fastify
 * @param {import("fastify").FastifyPluginOptions} options
 * @return {Promise<void>}
 */
const routes = async (fastify, options) => {
    fastify.post("/query/extract-ir", async (req, resp) => {
        const {orig_file_id: origFileId, msg_ix: msgIdx} = req.body;
        const sanitizedMsgIdx = Number(msgIdx);

        let irMetadata =
            await fastify.dbManager.getExtractedIrFileMetadata(origFileId, sanitizedMsgIdx);

        if (null === irMetadata) {
            const extractResult = await fastify.dbManager.submitAndWaitForExtractIrJob({
                file_split_id: null,
                msg_ix: sanitizedMsgIdx,
                orig_file_id: origFileId,
                // eslint-disable-next-line no-magic-numbers
                target_uncompressed_size: 128 * 1024 * 1024,
            });

            if (null === extractResult) {
                const err = new Error("Unable to extract IR for " +
                    `orig_file_id=${origFileId} at msg_ix=${sanitizedMsgIdx}`);

                err.statusCode = 400;
                throw err;
            }
            irMetadata =
                await fastify.dbManager.getExtractedIrFileMetadata(origFileId, sanitizedMsgIdx);
        }

        resp.send(irMetadata);
    });
};

export default routes;
