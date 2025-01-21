import {StatusCodes} from "http-status-codes";

import settings from "../../settings.json" with {type: "json"};
import {EXTRACT_JOB_TYPES} from "../DbManager.js";
import S3Manager from "../S3Manager.js";


const S3_MANAGER = (
    null === settings.StreamFilesS3PathPrefix ||
    null === settings.StreamFilesS3Region
) ?
    null :
    new S3Manager(settings.StreamFilesS3Region);


/**
 * Submits a stream extraction job and returns the metadata of the extracted stream.
 *
 * @param {object} props
 * @param {import("fastify").FastifyInstance | {dbManager: DbManager}} props.fastify
 * @param {EXTRACT_JOB_TYPES} props.jobType
 * @param {number} props.logEventIdx
 * @param {string} props.streamId
 * @param {import("fastify").FastifyReply} props.resp
 * @return {Promise<object>} A promise that resolves to the extracted stream's metadata.
 * @throws {Error} if the stream couldn't be extracted or its metadata doesn't exist in the
 * database.
 */
const extractStreamAndGetMetadata = async ({
    fastify,
    jobType,
    logEventIdx,
    streamId,
    resp,
}) => {
    const extractResult = await fastify.dbManager.submitAndWaitForExtractStreamJob({
        jobType: jobType,
        logEventIdx: logEventIdx,
        streamId: streamId,
        targetUncompressedSize: settings.StreamTargetUncompressedSize,
    });

    if (null === extractResult) {
        resp.code(StatusCodes.BAD_REQUEST);
        throw new Error(`Unable to extract stream with streamId=${streamId} at ` +
            `logEventIdx=${logEventIdx}`);
    }

    const streamMetadata = fastify.dbManager.getExtractedStreamFileMetadata(
        streamId,
        logEventIdx
    );

    if (null === streamMetadata) {
        resp.code(StatusCodes.BAD_REQUEST);
        throw new Error("Unable to find the metadata of extracted stream with " +
            `streamId=${streamId} at logEventIdx=${logEventIdx}`);
    }

    return streamMetadata;
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
            streamMetadata = await extractStreamAndGetMetadata({
                fastify: fastify,
                jobType: extractJobType,
                logEventIdx: sanitizedLogEventIdx,
                resp: resp,
                streamId: streamId,
            });
        }

        if (null === S3_MANAGER) {
            streamMetadata.path = `/streams/${streamMetadata.path}`;
        } else {
            streamMetadata.path = await S3_MANAGER.getPreSignedUrl(
                `s3://${settings.StreamFilesS3PathPrefix}${streamMetadata.path}`
            );
        }

        return streamMetadata;
    });
};

export default routes;
