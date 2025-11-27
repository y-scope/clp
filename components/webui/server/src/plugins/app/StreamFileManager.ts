import {QUERY_JOB_TYPE} from "@webui/common/query";
import {Nullable} from "@webui/common/utility-types";
import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";
import fp from "fastify-plugin";

import settings from "../../../settings.json" with {type: "json"};
import {
    StreamFileMetadata,
    StreamFilesCollection,
} from "../../typings/stream-files.js";


/**
 * Class to manage stream files for the log viewer.
 */
class StreamFileManager {
    readonly #queryJobDbManager: FastifyInstance["QueryJobDbManager"];

    readonly #logger: FastifyBaseLogger;

    readonly #streamFilesCollection: StreamFilesCollection;

    constructor ({QueryJobDbManager, logger, streamFilesCollection}: {
        QueryJobDbManager: FastifyInstance["QueryJobDbManager"];
        logger: FastifyBaseLogger;
        streamFilesCollection: StreamFilesCollection;
    }) {
        this.#queryJobDbManager = QueryJobDbManager;
        this.#logger = logger;
        this.#streamFilesCollection = streamFilesCollection;
    }

    static async create (app: FastifyInstance) {
        if ("undefined" === typeof app.mongo.db) {
            throw new Error("MongoDB database not found");
        }
        const streamFilesCollection =
            app.mongo.db.collection<StreamFileMetadata>(
                settings.MongoDbStreamFilesCollectionName
            );

        return new StreamFileManager({
            QueryJobDbManager: app.QueryJobDbManager,
            logger: app.log,
            streamFilesCollection: streamFilesCollection,
        });
    }

    /**
     * Submits a stream extraction job to the scheduler and waits for it to finish.
     *
     * @param props
     * @param props.dataset
     * @param props.jobType
     * @param props.logEventIdx
     * @param props.streamId
     * @param props.targetUncompressedSize
     * @return The ID of the job or null if an error occurred.
     */
    async submitAndWaitForExtractStreamJob ({
        dataset,
        jobType,
        logEventIdx,
        streamId,
        targetUncompressedSize,
    }: {
        dataset: string | null;
        jobType: QUERY_JOB_TYPE;
        logEventIdx: number;
        streamId: string;
        targetUncompressedSize: number;
    }): Promise<number | null> {
        let jobConfig;
        if (QUERY_JOB_TYPE.EXTRACT_IR === jobType) {
            jobConfig = {
                file_split_id: null,
                msg_ix: logEventIdx,
                orig_file_id: streamId,
                target_uncompressed_size: targetUncompressedSize,
            };
        } else if (QUERY_JOB_TYPE.EXTRACT_JSON === jobType) {
            jobConfig = {
                dataset: dataset,
                archive_id: streamId,
                target_chunk_size: targetUncompressedSize,
            };
        }

        if ("undefined" === typeof jobConfig) {
            throw new Error("Invalid job type");
        }

        try {
            return await this.#queryJobDbManager.submitAndWaitForJob(jobConfig, jobType);
        } catch (e) {
            this.#logger.error(e);

            return null;
        }
    }

    /**
     * Gets the metadata for the extracted stream that has the given streamId and contains the
     * given logEventIdx.
     *
     * @param streamId
     * @param logEventIdx
     * @return A promise that resolves to the extracted stream's metadata.
     */
    async getExtractedStreamFileMetadata (
        streamId: string,
        logEventIdx: number
    ): Promise<Nullable<StreamFileMetadata>> {
        return await this.#streamFilesCollection.findOne({
            stream_id: streamId,
            begin_msg_ix: {$lte: logEventIdx},
            end_msg_ix: {$gt: logEventIdx},
        });
    }
}

declare module "fastify" {
    interface FastifyInstance {
        StreamFileManager: StreamFileManager;
    }
}

export default fp(
    async (fastify) => {
        const streamFileManager = await StreamFileManager.create(fastify);
        fastify.decorate("StreamFileManager", streamFileManager);
    },
    {
        name: "StreamFileManager",
        dependencies: ["QueryJobDbManager"],
    }
);
