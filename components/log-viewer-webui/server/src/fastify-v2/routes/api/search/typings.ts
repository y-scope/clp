import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";

import {
    SEARCH_SIGNAL,
    SearchResultsMetadataDocument,
} from "../../../plugins/app/search/SearchResultsMetadataCollection/typings.js";


/**
 * The maximum number of results to retrieve for a search.
 */
const SEARCH_MAX_NUM_RESULTS = 1000;

type UpdateSearchResultsMetaProps = {
    jobId: number;
    lastSignal: SEARCH_SIGNAL;
    searchResultsMetadataCollection: FastifyInstance["SearchResultsMetadataCollection"];
    logger: FastifyBaseLogger;
    fields: Partial<SearchResultsMetadataDocument>;
};

type UpdateSearchSignalWhenJobsFinishProps = {
    searchJobId: number;
    aggregationJobId: number;
    queryJobsDbManager: FastifyInstance["QueryJobsDbManager"];
    searchJobCollectionsManager: FastifyInstance["SearchJobCollectionsManager"];
    searchResultsMetadataCollection: FastifyInstance["SearchResultsMetadataCollection"];
    logger: FastifyBaseLogger;
};

type CreateMongoIndexesProps = {
    searchJobId: number;
    searchJobCollectionsManager: FastifyInstance["SearchJobCollectionsManager"];
    logger: FastifyBaseLogger;
};

export {
    CreateMongoIndexesProps,
    SEARCH_MAX_NUM_RESULTS,
    UpdateSearchResultsMetaProps,
    UpdateSearchSignalWhenJobsFinishProps,
};
