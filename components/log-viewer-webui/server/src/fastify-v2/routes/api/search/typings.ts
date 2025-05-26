import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";

import {
    SEARCH_SIGNAL,
} from "../../../plugins/app/search/SearchResultsMetadataCollection/typings.js";


/**
 * MongoDB document for search results metadata. `numTotalResults` is optional
 * since it is only set when the search job is completed.
 */
interface SearchResultsMetadataDocument {
    _id: string;
    errorMsg: Nullable<string>;
    lastSignal: SEARCH_SIGNAL;
    numTotalResults?: number;
}

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
    SearchResultsMetadataDocument,
    UpdateSearchSignalWhenJobsFinishProps,
};
