import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

import settings from "../../../../../settings.json" with {type: "json"};
import {
    EXTRACT_JOB_TYPES,
    QUERY_JOB_TYPE,
} from "../../../../typings/query.js";
import {ErrorSchema} from "../../../schemas/error.js";

/**
 * Stream Files API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    fastify.post(
        "/extract",
        {
            schema: {
                body: Type.Object({
                    extractJobType: Type.Enum(QUERY_JOB_TYPE),
                    logEventIdx: Type.Integer(),
                    streamId: Type.String({minLength: 1}),
                }),
                response: {
                    [StatusCodes.OK]: Type.Object({
                        path: Type.String(),
                        stream_id: Type.String(),
                        begin_msg_ix: Type.Integer(),
                        end_msg_ix: Type.Integer(),
                    }),
                    [StatusCodes.BAD_REQUEST]: ErrorSchema,
                    [StatusCodes.INTERNAL_SERVER_ERROR]: ErrorSchema,
                },
                tags: ["Stream Files"],
            },
        },
        async (request, reply) => {
            const {extractJobType, logEventIdx, streamId} = request.body;

            if (false === EXTRACT_JOB_TYPES.has(extractJobType)) {
                return reply.badRequest(`Invalid extractJobType="${extractJobType}".`);
            }

            let streamMetadata = await fastify.streamFileManager.getExtractedStreamFileMetadata(
                streamId,
                logEventIdx
            );

            if (null === streamMetadata) {
                const extractResult = await fastify.streamFileManager.submitAndWaitForExtractStreamJob({
                    jobType: extractJobType,
                    logEventIdx: logEventIdx,
                    streamId: streamId,
                    targetUncompressedSize: settings.StreamTargetUncompressedSize,
                });

                if (null === extractResult) {
                    return reply.badRequest(
                        `Unable to extract stream with streamId=${streamId} at logEventIdx=${logEventIdx}`
                    );
                }

                streamMetadata = await fastify.streamFileManager.getExtractedStreamFileMetadata(
                    streamId,
                    logEventIdx
                );

                if (null === streamMetadata) {
                    return reply.badRequest(
                        `Unable to extract stream with streamId=${streamId} at logEventIdx=${logEventIdx}`
                    );
                }
            }

            if (fastify.hasDecorator("s3Manager") && "undefined" !== typeof fastify.s3Manager) {
                streamMetadata.path = await fastify.s3Manager.getPreSignedUrl(
                    `s3://${settings.StreamFilesS3PathPrefix}${streamMetadata.path}`
                );
            } else {
                streamMetadata.path = `/streams/${streamMetadata.path}`;
            }

            return streamMetadata;
        }
    );
};

export default plugin;
