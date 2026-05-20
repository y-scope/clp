import {type SearchResultsMetadataDocument} from "@webui/common/metadata";
import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";
import type {
    Collection,
    Db,
} from "mongodb";


/**
 * The maximum number of results to retrieve for a search.
 */
const DEFAULT_SEARCH_MAX_NUM_RESULTS = 1000;

type UpdateSearchSignalWhenJobsFinishProps = {
    aggregationJobId: number;
    logger: FastifyBaseLogger;
    maxNumResults: number;
    mongoDb: Db;
    queryJobDbManager: FastifyInstance["QueryJobDbManager"];
    searchJobId: number;
    searchResultsMetadataCollection: Collection<SearchResultsMetadataDocument>;

};

type CreateMongoIndexesProps = {
    searchJobId: number;
    logger: FastifyBaseLogger;
    mongoDb: Db;
};

export {
    CreateMongoIndexesProps,
    DEFAULT_SEARCH_MAX_NUM_RESULTS,
    UpdateSearchSignalWhenJobsFinishProps,
};
