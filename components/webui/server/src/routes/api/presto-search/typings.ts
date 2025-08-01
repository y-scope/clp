import type {FastifyBaseLogger} from "fastify";
import type {Db} from "mongodb";


/**
 * Props for inserting Presto rows into MongoDB.
 */
interface InsertPrestoRowsToMongoProps {
    data: unknown[][];
    columns: {name: string}[];
    searchJobId: string;
    isResolved: boolean;
    mongoDb: Db;
    log: FastifyBaseLogger;
}

export {InsertPrestoRowsToMongoProps};
