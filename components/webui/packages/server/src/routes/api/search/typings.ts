import {type SearchResultsMetadataDocument} from "@webui/common/metadata";
import {DEFAULT_MAX_NUM_SEARCH_RESULTS} from "@webui/common/schemas/search";
import {
    FastifyBaseLogger,
    FastifyInstance,
} from "fastify";
import type {
    Collection,
    Db,
} from "mongodb";


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
    DEFAULT_MAX_NUM_SEARCH_RESULTS,
    UpdateSearchSignalWhenJobsFinishProps,
};
