import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";

import {
    SEARCH_SIGNAL,
} from "@common/searchResultsMetadata.js";

import type {
    Collection,
    Db,
} from "mongodb";

import { Nullable } from "../../../../typings/common.js";

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
    searchResultsMetadataCollection: Collection<SearchResultsMetadataDocument>;
    logger: FastifyBaseLogger;
    fields: Partial<SearchResultsMetadataDocument>;
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
