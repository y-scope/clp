import type {FastifyBaseLogger} from "fastify";
import type {
    Collection,
    Db,
} from "mongodb";

import type {
    PRESTO_SEARCH_SIGNAL,
    SearchResultsMetadataDocument,
} from "../../../../../common/index.js";


interface InsertPrestoRowsToMongoProps {
    data: unknown[][];
    columns: {name: string}[];
    searchJobId: string;
    isResolved: boolean;
    mongoDb: Db;
    log: FastifyBaseLogger;
}

type ProcessPrestoStateChangeProps = {
    queryId: string;
    state: PRESTO_SEARCH_SIGNAL;
    isResolved: boolean;
    logger: FastifyBaseLogger;
    searchResultsMetadataCollection: Collection<SearchResultsMetadataDocument>;
    resolve: (value: string) => void;
};

export type {
    InsertPrestoRowsToMongoProps,
    ProcessPrestoStateChangeProps,
};
