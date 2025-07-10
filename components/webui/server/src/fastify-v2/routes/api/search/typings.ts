import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";
import type {
    Collection,
    Db,
} from "mongodb";

import {
    SEARCH_SIGNAL,
    type SearchResultsMetadataDocument,
} from "../../../../../../common/index.js";


/**
 * The maximum number of results to retrieve for a search.
 */
const SEARCH_MAX_NUM_RESULTS = 1000;

type UpdateSearchResultsMetaProps = {
    fields: Partial<SearchResultsMetadataDocument>;
    jobId: number;
    lastSignal: SEARCH_SIGNAL;
    logger: FastifyBaseLogger;
    searchResultsMetadataCollection: Collection<SearchResultsMetadataDocument>;
};

type UpdateSearchSignalWhenJobsFinishProps = {
    aggregationJobId: number;
    logger: FastifyBaseLogger;
    mongoDb: Db;
    queryJobsDbManager: FastifyInstance["QueryJobsDbManager"];
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
    UpdateSearchResultsMetaProps,
    UpdateSearchSignalWhenJobsFinishProps,
};
