import {
    Static,
    Type,
} from "@sinclair/typebox";

import {StringSchema} from "./common.js";


/**
 * Known CLP component names for operational logs.
 */
const CLP_COMPONENTS = [
    "api_server",
    "compression_scheduler",
    "compression_worker",
    "database",
    "garbage_collector",
    "log_ingestor",
    "mcp_server",
    "query_scheduler",
    "query_worker",
    "queue",
    "reducer",
    "redis",
    "results_cache",
    "webui",
] as const;

const ComponentNameSchema = Type.Union(
    CLP_COMPONENTS.map((c) => Type.Literal(c))
);

type ComponentName = Static<typeof ComponentNameSchema>;

/**
 * Schema for a component's log file metadata.
 */
const LogFileMetadataSchema = Type.Object({
    component: StringSchema,
    filename: StringSchema,
    path: StringSchema,
    sizeBytes: Type.Number(),
    modifiedTime: Type.Number(),
});

type LogFileMetadata = Static<typeof LogFileMetadataSchema>;

/**
 * Schema for listing available log files.
 */
const LogFilesListingSchema = Type.Array(LogFileMetadataSchema);

type LogFilesListing = Static<typeof LogFilesListingSchema>;

/**
 * Schema for listing components request.
 */
const ListComponentsRequestSchema = Type.Object({});

/**
 * Schema for listing components response.
 */
const ComponentsListingSchema = Type.Array(Type.Object({
    name: StringSchema,
    hasLogs: Type.Boolean(),
}));

type ComponentsListing = Static<typeof ComponentsListingSchema>;

/**
 * Schema for listing log files request.
 */
const ListLogFilesRequestSchema = Type.Object({
    component: Type.Optional(Type.String()),
});

/**
 * Schema for reading log content request.
 */
const ReadLogContentRequestSchema = Type.Object({
    path: StringSchema,
    offset: Type.Optional(Type.Number({minimum: 0})),
    limit: Type.Optional(Type.Number({minimum: 1, maximum: 10000})),
});

/**
 * Schema for a parsed log entry from Fluent Bit output.
 */
const LogEntrySchema = Type.Object({
    timestamp: Type.Number(),
    source: Type.String(),
    log: Type.String(),
    containerId: Type.Optional(Type.String()),
    containerName: Type.Optional(Type.String()),
    clpDeployment: Type.Optional(Type.String()),
});

type LogEntry = Static<typeof LogEntrySchema>;

/**
 * Schema for log content response.
 */
const LogContentResponseSchema = Type.Object({
    entries: Type.Array(LogEntrySchema),
    totalLines: Type.Number(),
    hasMore: Type.Boolean(),
});

type LogContentResponse = Static<typeof LogContentResponseSchema>;

export {
    CLP_COMPONENTS,
    ComponentNameSchema,
    ComponentsListingSchema,
    ListComponentsRequestSchema,
    ListLogFilesRequestSchema,
    LogContentResponseSchema,
    LogEntrySchema,
    LogFileMetadataSchema,
    LogFilesListingSchema,
    ReadLogContentRequestSchema,
};
export type {
    ComponentName,
    ComponentsListing,
    LogContentResponse,
    LogEntry,
    LogFileMetadata,
    LogFilesListing,
};
