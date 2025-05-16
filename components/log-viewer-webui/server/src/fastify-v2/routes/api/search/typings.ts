import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";

import {SEARCH_SIGNAL} from "../../../plugins/app/search/SearchResultsMetadataCollection/typings.js";
import {SearchResultsMetadataDocument} from "../../../plugins/app/search/SearchResultsMetadataCollection/typings.js";

type UpdateSearchResultsMetaProps = {
    jobId: number;
    lastSignal: SEARCH_SIGNAL;
    SearchResultsMetadataCollection: FastifyInstance["SearchResultsMetadataCollection"];
    logger: FastifyBaseLogger;
    fields: Partial<SearchResultsMetadataDocument>;
};

type UpdateSearchSignalWhenJobsFinishProps = {
    searchJobId: number;
    aggregationJobId: number;
    queryJobsDbManager: FastifyInstance["QueryJobsDbManager"];
    searchJobCollectionsManager: FastifyInstance["SearchJobCollectionsManager"];
    SearchResultsMetadataCollection: FastifyInstance["SearchResultsMetadataCollection"];
    logger: FastifyBaseLogger;
};

type CreateMongoIndexesProps = {
    searchJobId: number;
    searchJobCollectionsManager: FastifyInstance["SearchJobCollectionsManager"];
    logger: FastifyBaseLogger;
};

/**
 * The maximum number of results to retrieve for a search.
 */
const SEARCH_MAX_NUM_RESULTS = 1000;

export {
    CreateMongoIndexesProps,
    UpdateSearchResultsMetaProps,
    UpdateSearchSignalWhenJobsFinishProps,
    SEARCH_MAX_NUM_RESULTS,
};
