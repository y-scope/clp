import {
    Static,
    Type,
} from "@sinclair/typebox";
import {Collection} from "mongodb";


/**
 * Schema for stream file metadata.
 */
const StreamFileMetadataSchema = Type.Object({
    begin_msg_ix: Type.Integer(),
    end_msg_ix: Type.Integer(),
    is_last_chunk: Type.Boolean(),
    path: Type.String(),
    stream_id: Type.String(),
});

type StreamFileMetadata = Static<typeof StreamFileMetadataSchema>;

type StreamFilesCollection = Collection<StreamFileMetadata>;

export {StreamFileMetadataSchema};
export type {
    StreamFileMetadata,
    StreamFilesCollection,
};
