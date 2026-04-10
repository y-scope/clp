import {
    CLP_DEFAULT_DATASET_NAME,
    CLP_STORAGE_ENGINES,
} from "@webui/common/config";
import {
    CompressionJobCreation,
    CompressionJobInputType,
    S3CompressionJobCreation,
} from "@webui/common/schemas/compression";

import {SETTINGS_STORAGE_ENGINE} from "../../../config";


type ClpSPayload = Pick<CompressionJobCreation, "dataset" | "timestampKey" | "unstructured">;

type S3PathsPayload = Pick<S3CompressionJobCreation, "keyPrefix" | "keys">;

type ClpSFormValues = Pick<
    S3CompressionJobCreation,
    "bufferChannelCapacity" |
    "bufferFlushThresholdBytes" |
    "bufferTimeoutSec" |
    "dataset" |
    "scanningIntervalSec" |
    "timestampKey" |
    "unstructured"
>;


/**
 * Applies CLP-S fields to a job payload.
 *
 * @param payload
 * @param payload.dataset
 * @param payload.timestampKey
 * @param payload.unstructured
 * @param values
 */
const applyClpSFields = (
    payload: ClpSPayload,
    values: ClpSFormValues,
) => {
    if ("undefined" === typeof values.dataset || 0 === values.dataset.length) {
        payload.dataset = CLP_DEFAULT_DATASET_NAME;
    } else {
        payload.dataset = values.dataset;
    }
    if (true === values.unstructured) {
        payload.unstructured = true;
    } else if ("undefined" !== typeof values.timestampKey) {
        payload.timestampKey = values.timestampKey;
    }
};

/**
 * Parses a list of selected S3 paths into keyPrefix/keys fields.
 *
 * Selection semantics:
 * - If a single prefix (ending with "/") is selected: use `keyPrefix` mode.
 * - If the bucket root ("") is selected: use `keyPrefix` with empty string.
 * - If only object keys are selected: use `keys` mode.
 *
 * @param s3Payload
 * @param s3Payload.keyPrefix
 * @param s3Payload.keys
 * @param s3Paths Selected values from the S3 TreeSelect.
 */
const applyS3Paths = (
    s3Payload: S3PathsPayload,
    s3Paths: string[],
) => {
    // Separate prefixes (ending with "/" or empty string for root) from object keys.
    const prefixes = s3Paths.filter((p) => p.endsWith("/") || "" === p);
    const objectKeys = s3Paths.filter((p) => false === p.endsWith("/") && "" !== p);

    if (0 < prefixes.length && 0 === objectKeys.length) {
        const [firstPrefix] = prefixes;
        s3Payload.keyPrefix = firstPrefix ?? "";
    } else if (0 === prefixes.length && 0 < objectKeys.length) {
        s3Payload.keys = objectKeys;
    } else {
        if (0 < prefixes.length) {
            s3Payload.keyPrefix = prefixes[0] as string;
        }
        if (0 < objectKeys.length) {
            s3Payload.keys = objectKeys;
        }
    }
};

interface BuildS3PayloadArgs {
    bucket: string;
    regionCode: string;
    s3Paths: string[] | undefined;
    scanner: boolean;
    values: ClpSFormValues;
}

/**
 * Builds an S3 job payload from form values.
 *
 * @param args
 * @param args.bucket
 * @param args.regionCode
 * @param args.scanner Whether this is a scanner job.
 * @param args.s3Paths
 * @param args.values
 * @return
 */
const buildS3Payload = (args: BuildS3PayloadArgs): S3CompressionJobCreation => {
    const {bucket, regionCode, s3Paths, scanner, values} = args;
    const payload: S3CompressionJobCreation = {
        bucket: bucket,
        inputType: CompressionJobInputType.S3,
        regionCode: regionCode,
        scanner: scanner,
    };

    if (s3Paths && 0 < s3Paths.length) {
        applyS3Paths(payload, s3Paths);
    }
    if (scanner) {
        if ("number" === typeof values.scanningIntervalSec) {
            payload.scanningIntervalSec = values.scanningIntervalSec;
        }
        if ("number" === typeof values.bufferFlushThresholdBytes) {
            payload.bufferFlushThresholdBytes = values.bufferFlushThresholdBytes;
        }
        if ("number" === typeof values.bufferTimeoutSec) {
            payload.bufferTimeoutSec = values.bufferTimeoutSec;
        }
        if ("number" === typeof values.bufferChannelCapacity) {
            payload.bufferChannelCapacity = values.bufferChannelCapacity;
        }
    }
    if (CLP_STORAGE_ENGINES.CLP_S === SETTINGS_STORAGE_ENGINE) {
        applyClpSFields(payload, values);
    }

    return payload;
};


export {
    applyClpSFields,
    applyS3Paths,
    buildS3Payload,
};
