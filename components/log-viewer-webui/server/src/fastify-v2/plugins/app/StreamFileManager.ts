import {
    FastifyInstance,
    FastifyPluginAsync,
    FastifyBaseLogger,
} from "fastify";
import {fastifyPlugin} from "fastify-plugin";

import {Nullable} from "../../../typings/common.js";
import {
    DbManagerOptions,
    StreamFileMongoDocument,
    StreamFilesCollection,
} from "../../../typings/DbManager.js";
import {QUERY_JOB_TYPE} from "../../../typings/query.js";
import {JobManager} from "./JobManager/index.js";

/**
 * Class to manage stream files for the log viewer.
 */
class StreamFileManager {
    readonly #jobManager: JobManager;
    readonly #logger: FastifyBaseLogger;
    readonly #streamFilesCollection: StreamFilesCollection;

    constructor ({jobManager, logger, streamFilesCollection}: {
        jobManager: JobManager;
        logger: FastifyBaseLogger;
        streamFilesCollection: StreamFilesCollection;
    }) {
        this.#jobManager = jobManager;
        this.#logger = logger;
        this.#streamFilesCollection = streamFilesCollection;
    }

    static async create (app: FastifyInstance, dbConfig: DbManagerOptions) {
        if ("undefined" === typeof app.mongo.db) {
            throw new Error("MongoDB database not found");
        }
        const streamFilesCollection =
            app.mongo.db.collection<StreamFileMongoDocument>(
                dbConfig.mongoConfig.streamFilesCollectionName
            );

        return new StreamFileManager({
            jobManager: app.jobManager,
            logger: app.log,
            streamFilesCollection: streamFilesCollection,
        });
    }

    /**
     * Submits a stream extraction job to the scheduler and waits for it to finish.
     *
     * @param props
     * @param props.jobType
     * @param props.logEventIdx
     * @param props.streamId
     * @param props.targetUncompressedSize
     * @return The ID of the job or null if an error occurred.
     */
    async submitAndWaitForExtractStreamJob ({
        jobType,
        logEventIdx,
        streamId,
        targetUncompressedSize,
    }: {
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
        } else {
            jobConfig = {
                archive_id: streamId,
                target_chunk_size: targetUncompressedSize,
            };
        }

        try {
            return await this.#jobManager.submitAndWaitForJob(jobConfig, jobType);
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
    async getExtractedStreamFileMetadata (streamId: string, logEventIdx: number)
        : Promise<Nullable<StreamFileMongoDocument>> {
        return await this.#streamFilesCollection.findOne({
            stream_id: streamId,
            begin_msg_ix: {$lte: logEventIdx},
            end_msg_ix: {$gt: logEventIdx},
        });
    }
}

const streamFileManagerPluginCallback: FastifyPluginAsync<DbManagerOptions> = async (app, options) => {
    const streamFileManager = await StreamFileManager.create(app, options);
    app.decorate("streamFileManager", streamFileManager);
};

declare module "fastify" {
    interface FastifyInstance {
        streamFileManager: StreamFileManager;
    }
}

export {QUERY_JOB_TYPE};
export default fastifyPlugin(streamFileManagerPluginCallback, {
    dependencies: ["jobManager"],
});
