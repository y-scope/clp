import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {StatusCodes} from "http-status-codes";

import settings from "../../../../settings.json" with {type: "json"};
import {ErrorSchema} from "../../../schemas/error.js";
import {StreamFileExtractionSchema} from "../../../schemas/stream-files.js";
import {EXTRACT_JOB_TYPES} from "../../../typings/query.js";
import {StreamFileMetadataSchema} from "../../../typings/stream-files.js";


/**
 * Stream Files API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const {StreamFileManager} = fastify;

    fastify.post(
        "/extract",
        {
            schema: {
                body: StreamFileExtractionSchema,
                response: {
                    [StatusCodes.OK]: StreamFileMetadataSchema,
                    [StatusCodes.BAD_REQUEST]: ErrorSchema,
                },
                tags: ["Stream Files"],
            },
        },
        async (request, reply) => {
            const {dataset, extractJobType, logEventIdx, streamId} = request.body;

            if (false === EXTRACT_JOB_TYPES.has(extractJobType)) {
                return reply.badRequest(`Invalid extractJobType="${extractJobType}".`);
            }

            let streamMetadata = await StreamFileManager.getExtractedStreamFileMetadata(
                streamId,
                logEventIdx
            );

            if (null === streamMetadata) {
                const extractResult = await StreamFileManager.submitAndWaitForExtractStreamJob({
                    dataset: dataset,
                    jobType: extractJobType,
                    logEventIdx: logEventIdx,
                    streamId: streamId,
                    targetUncompressedSize: settings.StreamTargetUncompressedSize,
                });

                if (null === extractResult) {
                    return reply.badRequest(
                        `Unable to extract stream with streamId=${streamId} at ` +
                        `logEventIdx=${logEventIdx}`
                    );
                }

                streamMetadata = await StreamFileManager.getExtractedStreamFileMetadata(
                    streamId,
                    logEventIdx
                );

                if (null === streamMetadata) {
                    return reply.badRequest(
                        `Unable to extract stream with streamId=${streamId} at ` +
                        `logEventIdx=${logEventIdx}`
                    );
                }
            }

            if (fastify.hasDecorator("S3Manager") && "undefined" !== typeof fastify.S3Manager) {
                streamMetadata.path = await fastify.S3Manager.getPreSignedUrl(
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
