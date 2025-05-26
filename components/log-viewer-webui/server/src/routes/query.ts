import {TypeBoxTypeProvider} from "@fastify/type-provider-typebox";
import {Type} from "@sinclair/typebox";
import {FastifyPluginAsync} from "fastify";
import {StatusCodes} from "http-status-codes";

import settings from "../../settings.json" with {type: "json"};
import {
    EXTRACT_JOB_TYPES,
    QUERY_JOB_TYPE,
} from "../typings/query.js";


/**
 * Creates query routes.
 *
 * @param app
 * @return
 */
// eslint-disable-next-line max-lines-per-function
const routes: FastifyPluginAsync = async (app) => {
    const fastify = app.withTypeProvider<TypeBoxTypeProvider>();

    fastify.post(
        "/query/sql",
        {
            schema: {
                body: Type.Object({
                    queryString: Type.String({minLength: 1}),
                }),
            },
        },
        async (req) => {
            const {queryString} = req.body;
            return await fastify.dbManager.queryMySql(queryString);
        },
    );

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
            const extractResult = await fastify.dbManager.submitAndWaitForExtractStreamJob({
                jobType: extractJobType,
                logEventIdx: logEventIdx,
                streamId: streamId,
                targetUncompressedSize: settings.StreamTargetUncompressedSize,
            });

            if (null === extractResult) {
                resp.code(StatusCodes.BAD_REQUEST);
                throw new Error(`Unable to extract stream with streamId=${streamId} at ` +
            `logEventIdx=${logEventIdx}`);
            }

            streamMetadata = await fastify.dbManager.getExtractedStreamFileMetadata(
                streamId,
                logEventIdx
            );
            if (null === streamMetadata) {
                resp.code(StatusCodes.BAD_REQUEST);
                throw new Error("Unable to extract stream with " +
                    `streamId=${streamId} at logEventIdx=${logEventIdx}`);
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
    });
};

export default routes;
