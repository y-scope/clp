import {TypeBoxTypeProvider} from "@fastify/type-provider-typebox";
import {Type} from "@sinclair/typebox";
import {FastifyPluginAsync} from "fastify";
import {StatusCodes} from "http-status-codes";

import settings from "../../settings.json" with {type: "json"};
import {QUERY_JOB_TYPE} from "../DbManager.js";
import {EXTRACT_JOB_TYPES} from "../typings/DbManager.js";


/**
 * Submits a stream extraction job and returns the metadata of the extracted stream.
 *
 * @param {object} props
 * @param {import("fastify").FastifyInstance |
 * {dbManager: DbManager} |
 * {s3Manager: S3Manager}} props.fastify
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
 * @param app
 */
const routes: FastifyPluginAsync = async (app) => {
    const fastify = app.withTypeProvider<TypeBoxTypeProvider>();

    fastify.post("/query/extract-stream", {
        schema: {
            body: Type.Object({
                extractJobType: Type.Enum(QUERY_JOB_TYPE),
                logEventIdx: Type.Integer(),
                streamId: Type.String({minLength: 1}),
            }),
        },
    }, async (req, resp) => {
        const {extractJobType, logEventIdx, streamId} = req.body;
        if (false === EXTRACT_JOB_TYPES.has(extractJobType)) {
            resp.code(StatusCodes.BAD_REQUEST);
            throw new Error(`Invalid extractJobType="${extractJobType}".`);
        }

        let streamMetadata = await fastify.dbManager.getExtractedStreamFileMetadata(
            streamId,
            logEventIdx
        );

        if (null === streamMetadata) {
            streamMetadata = await extractStreamAndGetMetadata({
                fastify: fastify,
                jobType: extractJobType,
                logEventIdx: logEventIdx,
                logEventIdx: sanitizedLogEventIdx,
                resp: resp,
                streamId: streamId,
            });

            if (null === extractResult) {
                resp.code(StatusCodes.BAD_REQUEST);
                throw new Error("Unable to extract stream with " +
                    `streamId=${streamId} at logEventIdx=${logEventIdx}`);
            }
        }

            streamMetadata = await fastify.dbManager.getExtractedStreamFileMetadata(
                streamId,
                logEventIdx
        if (fastify.hasDecorator("s3Manager")) {
            streamMetadata.path = await fastify.s3Manager.getPreSignedUrl(
                `s3://${settings.StreamFilesS3PathPrefix}${streamMetadata.path}`
            );

            if (null === streamMetadata) {
                resp.code(StatusCodes.BAD_REQUEST);
                throw new Error("Unable to find the metadata of extracted stream with " +
                    `streamId=${streamId} at logEventIdx=${logEventIdx}`);
            }
        } else {
            streamMetadata.path = `/streams/${streamMetadata.path}`;
        }

        return streamMetadata;
    });
};

export default routes;
