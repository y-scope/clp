import {
    Static,
    Type,
} from "@sinclair/typebox";

import {StringSchema} from "./common.js";


/**
 * Matching `AwsAuthType` in `clp_py_utils.clp_config`.
 */
enum AwsAuthType {
    DEFAULT = "default",
    CREDENTIALS = "credentials",
    PROFILE = "profile",
    ENV_VARS = "env_vars",
}

/**
 * Matching `S3Credentials` in `clp_py_utils.clp_config`.
 */
const S3CredentialsSchema = Type.Object({
    access_key_id: Type.String(),
    secret_access_key: Type.String(),
    session_token: Type.Union([
        Type.String(),
        Type.Null(),
    ]),
});

/**
 * Matching `AwsAuthentication` in `clp_py_utils.clp_config`.
 */
const AwsAuthenticationSchema = Type.Object({
    credentials: Type.Union([
        S3CredentialsSchema,
        Type.Null(),
    ]),
    profile: Type.Union([
        Type.String(),
        Type.Null(),
    ]),
    type: Type.Union([
        Type.Literal(AwsAuthType.DEFAULT),
        Type.Literal(AwsAuthType.CREDENTIALS),
        Type.Literal(AwsAuthType.PROFILE),
        Type.Literal(AwsAuthType.ENV_VARS),
    ]),
});

type AwsAuthentication = Static<typeof AwsAuthenticationSchema>;

/**
 * Schema for S3 listing request query parameters.
 */
const S3ListRequestSchema = Type.Object({
    bucket: Type.String({minLength: 1}),
    continuationToken: Type.Optional(Type.String()),
    prefix: Type.Optional(Type.String()),
    region: Type.String({minLength: 1}),
});

type S3ListRequest = Static<typeof S3ListRequestSchema>;

/**
 * Schema for an S3 object or common prefix entry.
 */
const S3EntrySchema = Type.Object({
    isPrefix: Type.Boolean(),
    key: StringSchema,
});

type S3Entry = Static<typeof S3EntrySchema>;

/**
 * Schema for S3 listing response.
 */
const S3ListResponseSchema = Type.Object({
    entries: Type.Array(S3EntrySchema),
    isTruncated: Type.Boolean(),
    nextContinuationToken: Type.Union([Type.String(),
        Type.Null()]),
});

type S3ListResponse = Static<typeof S3ListResponseSchema>;

export {
    AwsAuthenticationSchema,
    AwsAuthType,
    S3EntrySchema,
    S3ListRequestSchema,
    S3ListResponseSchema,
};
export type {
    AwsAuthentication,
    S3Entry,
    S3ListRequest,
    S3ListResponse,
};
