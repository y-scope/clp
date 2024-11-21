import {StatusCodes} from "http-status-codes";

import settings from "../../settings.json" with {type: "json"};
import {EXTRACT_JOB_TYPES} from "../DbManager.js";


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

        if (false === EXTRACT_JOB_TYPES.includes(extractJobType)) {
            resp.code(StatusCodes.BAD_REQUEST);
            throw new Error("Invalid extractJobType");
        }

        if (null === streamId) {
            resp.code(StatusCodes.BAD_REQUEST);
            throw new Error("streamId must not be null");
        }

        let streamMetadata = await fastify.dbManager.getExtractedStreamFileMetadata(
            streamId,
            sanitizedLogEventIdx
        );

        if (null === streamMetadata) {
            const extractResult = await fastify.dbManager.submitAndWaitForExtractStreamJob(
                extractJobType,
                sanitizedLogEventIdx,
                streamId,
                settings.StreamTargetUncompressedSize,
            );

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

        return streamMetadata;
    });
};

export default routes;
