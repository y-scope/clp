import {Collection} from "mongodb";


interface StreamFileMongoDocument {
    path: string;
    stream_id: string;
    begin_msg_ix: number;
    end_msg_ix: number;
    is_last_chunk: boolean;
}

type StreamFilesCollection = Collection<StreamFileMongoDocument>;


export type {
    StreamFileMongoDocument,
    StreamFilesCollection,
};
