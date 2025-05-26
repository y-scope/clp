import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";

import {
    SEARCH_SIGNAL,
    SearchResultsMetadataDocument,
} from "@common/searchResultsMetadata.js";

import type {
    Collection,
    Db,
} from "mongodb";

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
    mongoDb: Db,
    queryJobsDbManager: FastifyInstance["QueryJobsDbManager"];
    searchJobId: number;
    searchResultsMetadataCollection: Collection<SearchResultsMetadataDocument>;

};

type CreateMongoIndexesProps = {
    searchJobId: number;
    logger: FastifyBaseLogger;
    mongoDb: Db,
};

export {
    CreateMongoIndexesProps,
    SEARCH_MAX_NUM_RESULTS,
    UpdateSearchResultsMetaProps,
    SearchResultsMetadataDocument,
    UpdateSearchSignalWhenJobsFinishProps,
};
