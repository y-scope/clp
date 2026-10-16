import {
    type Static,
    Type,
} from "@sinclair/typebox";
import {Value} from "@sinclair/typebox/value";

import {
    CLP_DEFAULT_DATASET_NAME,
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "../config.js";
import {DatasetNameSchema} from "./compression.js";
import type {WebuiPublicSettings} from "./settings.js";


const requiredInputFields = {
    bucket: Type.String({minLength: 1, pattern: "\\S"}),
    keyPrefix: Type.String({minLength: 1}),
    region: Type.String({minLength: 1, pattern: "\\S"}),
};

const S3ScannerCreationSchema = Type.Object({
    ...requiredInputFields,
    dataset: Type.Optional(Type.Union([Type.Literal(""),
        DatasetNameSchema])),
    timestampKey: Type.Optional(Type.String()),
    unstructured: Type.Optional(Type.Boolean()),
}, {additionalProperties: false});

type S3ScannerCreation = Static<typeof S3ScannerCreationSchema>;

const S3ScannerJobSchema = Type.Object({
    id: Type.Integer({minimum: 1, maximum: Number.MAX_SAFE_INTEGER}),
});

type S3ScannerJob = Static<typeof S3ScannerJobSchema>;

const S3ScannerErrorSchema = Type.Object({error: Type.String()});

/**
 * Checks the public deployment capability without requiring AWS credential fields.
 *
 * @param settings
 * @return Whether the deployment supports scanner submission.
 */
const isS3ScannerEnabled = (settings: Pick<WebuiPublicSettings,
    "ClpStorageEngine" | "LogsInputType" | "S3ScannerEnabled">): boolean => {
    return CLP_STORAGE_ENGINES.CLP_S === settings.ClpStorageEngine &&
        STORAGE_TYPE.S3 === settings.LogsInputType && settings.S3ScannerEnabled;
};

/**
 * Uses the request schema for the form's required S3 inputs.
 *
 * @param field
 * @param value
 * @return An error message, or null for valid input.
 */
const validateS3ScannerInput = (
    field: keyof typeof requiredInputFields,
    value: unknown,
): string | null => {
    if (Value.Check(requiredInputFields[field], value)) {
        return null;
    }

    return {
        bucket: "Enter an S3 bucket name.",
        keyPrefix: "Enter a nonempty S3 key prefix.",
        region: "Enter an AWS region.",
    }[field];
};

/**
 * Maps allowed form values to the existing log-ingestor request contract.
 *
 * @param input
 * @return A scanner config using native defaults for omitted tuning fields.
 */
const toS3ScannerConfig = (input: S3ScannerCreation) => ({
    bucket_name: input.bucket,
    dataset: input.dataset || CLP_DEFAULT_DATASET_NAME,
    key_prefix: input.keyPrefix,
    region: input.region,
    ...(!input.unstructured && input.timestampKey ?
        {timestamp_key: input.timestampKey} :
        {}),
    unstructured: input.unstructured ?? false,
});


export {
    isS3ScannerEnabled,
    S3ScannerCreationSchema,
    S3ScannerErrorSchema,
    S3ScannerJobSchema,
    toS3ScannerConfig,
    validateS3ScannerInput,
};
export type {
    S3ScannerCreation, S3ScannerJob,
};
