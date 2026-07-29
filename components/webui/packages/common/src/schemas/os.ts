import {
    Static,
    Type,
} from "@sinclair/typebox";

import {StringSchema} from "./common.js";


/**
 * Schema for a file or directory entry.
 */
const FileEntrySchema = Type.Object({
    isExpandable: Type.Boolean(),
    name: StringSchema,
    parentPath: StringSchema,
});

type FileEntry = Static<typeof FileEntrySchema>;

const FileListRequestSchema = Type.Object({
    path: Type.String({
        default: "/",
    }),
});

/**
 * Schema for file listing response.
 */
const FileListingSchema = Type.Array(FileEntrySchema);

type FileListing = Static<typeof FileListingSchema>;

export {
    FileEntrySchema,
    FileListingSchema,
    FileListRequestSchema,
};
export type {
    FileEntry,
    FileListing,
};
