import {
    Static,
    Type,
} from "@sinclair/typebox";

import {StringSchema} from "./common.js";


/**
 * Schema for a file or directory entry.
 */
const FileEntrySchema = Type.Object({
    is_expandable: Type.Boolean(),
    name: StringSchema,
    parent_path: StringSchema,
});

type FileEntry = Static<typeof FileEntrySchema>;

/**
 * Schema for file listing response.
 */
const FileListingSchema = Type.Array(FileEntrySchema);

type FileListing = Static<typeof FileListingSchema>;

export {
    FileEntrySchema,
    FileListingSchema,
};
export type {
    FileEntry,
    FileListing,
};
