import {Collection} from "mongodb";


interface DbManagerOptions {
    mysqlConfig: {
        user: string;
        password: string;
        host: string;
        port: number;
        database: string;
        queryJobsTableName: string;
    };
    mongoConfig: {
        database: string;
        host: string;
        streamFilesCollectionName: string;
        port: number;
    };
}

interface StreamFileMongoDocument {
    path: string;
    stream_id: string;
    begin_msg_ix: number;
    end_msg_ix: number;
    is_last_chunk: boolean;
}

type StreamFilesCollection = Collection<StreamFileMongoDocument>;


export type {
    DbManagerOptions,
    StreamFileMongoDocument,
    StreamFilesCollection,
};
