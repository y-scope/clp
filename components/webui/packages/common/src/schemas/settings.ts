import {
    Static,
    Type,
} from "@sinclair/typebox";

import {
    CLP_QUERY_ENGINES,
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "../config.js";


const WebuiPublicSettingsSchema = Type.Object({
    ClpQueryEngine: Type.Enum(CLP_QUERY_ENGINES),
    ClpStorageEngine: Type.Enum(CLP_STORAGE_ENGINES),

    LogsInputRootDir: Type.Union([
        Type.String(),
        Type.Null(),
    ]),
    LogsInputType: Type.Enum(STORAGE_TYPE),

    MaxDatasetsPerQuery: Type.Union([
        Type.Integer({minimum: 1}),
        Type.Null(),
    ]),

    PrestoMaxNumSearchResults: Type.Integer({minimum: 1}),

    SqlDbClpArchivesTableName: Type.String(),
    SqlDbClpDatasetsTableName: Type.String(),
    SqlDbClpFilesTableName: Type.String(),
    SqlDbClpTablePrefix: Type.String(),
    SqlDbCompressionJobsTableName: Type.String(),

    MongoDbSearchResultsMetadataCollectionName: Type.String(),
});

const WebuiServerSettingsSchema = Type.Object({
    SqlDbHost: Type.String(),
    SqlDbName: Type.String(),
    SqlDbPort: Type.Integer({minimum: 1, maximum: 65535}),

    SqlDbQueryJobsTableName: Type.String(),

    MongoDbHost: Type.String(),
    MongoDbName: Type.String(),
    MongoDbPort: Type.Integer({minimum: 1, maximum: 65535}),

    MongoDbStreamFilesCollectionName: Type.String(),

    ClientDir: Type.String(),
    LogViewerDir: Type.String(),
    StreamFilesDir: Type.Union([
        Type.String(),
        Type.Null(),
    ]),

    StreamFilesS3PathPrefix: Type.Union([
        Type.String(),
        Type.Null(),
    ]),
    StreamFilesS3Profile: Type.Union([
        Type.String(),
        Type.Null(),
    ]),
    StreamFilesS3Region: Type.Union([
        Type.String(),
        Type.Null(),
    ]),
    StreamTargetUncompressedSize: Type.Integer({minimum: 1}),

    ArchiveOutputCompressionLevel: Type.Integer({minimum: 1, maximum: 19}),
    ArchiveOutputTargetArchiveSize: Type.Integer({minimum: 1}),
    ArchiveOutputTargetDictionariesSize: Type.Integer({minimum: 1}),
    ArchiveOutputTargetEncodedFileSize: Type.Integer({minimum: 1}),
    ArchiveOutputTargetSegmentSize: Type.Integer({minimum: 1}),

    PrestoHost: Type.Union([
        Type.String(),
        Type.Null(),
    ]),
    PrestoPort: Type.Union([
        Type.Integer({minimum: 1, maximum: 65535}),
        Type.Null(),
    ]),
});

const WebuiSettingsSchema = Type.Object({
    public: WebuiPublicSettingsSchema,
    server: WebuiServerSettingsSchema,
});

type WebuiPublicSettings = Static<typeof WebuiPublicSettingsSchema>;

type WebuiServerSettings = Static<typeof WebuiServerSettingsSchema>;

type WebuiSettings = Static<typeof WebuiSettingsSchema>;

export {
    WebuiPublicSettingsSchema,
    WebuiServerSettingsSchema,
    WebuiSettingsSchema,
};
export type {
    WebuiPublicSettings,
    WebuiServerSettings,
    WebuiSettings,
};
