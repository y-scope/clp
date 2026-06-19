import {
    Static,
    Type,
} from "@sinclair/typebox";


const WebuiPublicSettingsSchema = Type.Object({
    ClpQueryEngine: Type.String(),
    ClpStorageEngine: Type.String(),

    LogsInputRootDir: Type.Union([
        Type.String(),
        Type.Null(),
    ]),
    LogsInputType: Type.String(),

    MaxDatasetsPerQuery: Type.Union([
        Type.Number(),
        Type.Null(),
    ]),

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
    StreamTargetUncompressedSize: Type.Number(),

    ArchiveOutputCompressionLevel: Type.Number(),
    ArchiveOutputTargetArchiveSize: Type.Number(),
    ArchiveOutputTargetDictionariesSize: Type.Number(),
    ArchiveOutputTargetEncodedFileSize: Type.Number(),
    ArchiveOutputTargetSegmentSize: Type.Number(),

    PrestoHost: Type.Union([
        Type.String(),
        Type.Null(),
    ]),
    PrestoPort: Type.Union([
        Type.Integer(),
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
