import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";
import type {
    Collection,
    Db,
} from "mongodb";

import {type SearchResultsMetadataDocument} from "../../../../../common/index.js";


/**
 * The maximum number of results to retrieve for a search.
 */
const SEARCH_MAX_NUM_RESULTS = 1000;

type UpdateSearchSignalWhenJobsFinishProps = {
    aggregationJobId: number;
    logger: FastifyBaseLogger;
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
    SEARCH_MAX_NUM_RESULTS,
    SearchResultsMetadataDocument,
    UpdateSearchSignalWhenJobsFinishProps,
};
