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

    MongoDbSearchResultsMetadataCollectionName: Type.String(),
});

const WebuiServerSettingsSchema = Type.Object({
    /**
     * Base URL of the API server. Always set: `validate_webui_config` in
     * `clp_package_utils.general` and the Helm chart's `webui-deployment.yaml` both reject a
     * deployment whose `api_server` is null.
     */
    ApiServerUrl: Type.String(),

    MongoDbHost: Type.String(),
    MongoDbName: Type.String(),
    MongoDbPort: Type.Integer({minimum: 1, maximum: 65535}),

    ClientDir: Type.String(),
    LogViewerDir: Type.String(),
    StreamFilesDir: Type.Union([
        Type.String(),
        Type.Null(),
    ]),

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
