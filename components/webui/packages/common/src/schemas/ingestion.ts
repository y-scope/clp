import {
    type Static,
    Type,
} from "@sinclair/typebox";
import {Value} from "@sinclair/typebox/value";

import {DatasetNameSchema} from "./compression.js";


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

const S3ScannerConfigSchema = Type.Object({
    bucket_name: requiredInputFields.bucket,
    dataset: Type.Optional(DatasetNameSchema),
    key_prefix: requiredInputFields.keyPrefix,
    region: requiredInputFields.region,
    timestamp_key: Type.Optional(Type.String({minLength: 1})),
    unstructured: Type.Optional(Type.Boolean()),
}, {additionalProperties: false});

type S3ScannerConfig = Static<typeof S3ScannerConfigSchema>;

const S3ScannerJobSchema = Type.Object({
    id: Type.Integer({minimum: 1, maximum: Number.MAX_SAFE_INTEGER}),
});

type S3ScannerJob = Static<typeof S3ScannerJobSchema>;

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

export {
    S3ScannerConfigSchema,
    S3ScannerCreationSchema,
    S3ScannerJobSchema,
    validateS3ScannerInput,
};
export type {
    S3ScannerConfig, S3ScannerCreation, S3ScannerJob,
};
