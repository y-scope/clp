import {Value} from "@sinclair/typebox/value";
import {DATASET_NAME_MAX_LEN} from "@webui/common/schemas/compression";
import {
    isS3ScannerEnabled,
    S3ScannerCreationSchema,
    toS3ScannerConfig,
    validateS3ScannerInput,
} from "@webui/common/schemas/ingestion";
import {
    type WebuiSettings,
    WebuiSettingsSchema,
} from "@webui/common/schemas/settings";
import tap from "tap";

import {enabledSettings} from "./s3-scanner.helpers.js";


const input = {bucket: "test-bucket", keyPrefix: "logs/", region: "us-east-2"};

tap.test("shared scanner validation and native mapping", (t) => {
    t.ok(Value.Check(S3ScannerCreationSchema, input));
    t.equal(validateS3ScannerInput("bucket", "  "), "Enter an S3 bucket name.");
    t.equal(validateS3ScannerInput("region", "\t"), "Enter an AWS region.");
    t.equal(validateS3ScannerInput("keyPrefix", ""), "Enter a nonempty S3 key prefix.");
    t.equal(validateS3ScannerInput("keyPrefix", " /literal//prefix "), null);
    const maxDataset = "a".repeat(DATASET_NAME_MAX_LEN);
    t.ok(Value.Check(S3ScannerCreationSchema, {...input, dataset: maxDataset}));
    t.notOk(Value.Check(S3ScannerCreationSchema, {...input, dataset: `${maxDataset}a`}));
    t.notOk(Value.Check(S3ScannerCreationSchema, {...input, dataset: "bad-name"}));
    t.notOk(Value.Check(S3ScannerCreationSchema, {...input, dataset: "中文"}));
    t.same(toS3ScannerConfig({...input, keyPrefix: " /literal//prefix "}), {
        bucket_name: input.bucket,
        dataset: "default",
        key_prefix: " /literal//prefix ",
        region: input.region,
        unstructured: false,
    });
    t.equal(toS3ScannerConfig({...input, dataset: ""}).dataset, "default");
    t.notOk(Object.hasOwn(toS3ScannerConfig({...input, timestampKey: ""}), "timestamp_key"));
    t.notOk(Object.hasOwn(toS3ScannerConfig({
        ...input, timestampKey: "stale", unstructured: true,
    }), "timestamp_key"));
    t.equal(toS3ScannerConfig({...input, timestampKey: "x.y"}).timestamp_key, "x.y");
    t.end();
});

tap.test("old settings disable scanners without a migration", (t) => {
    const configuration = enabledSettings();
    const legacyPublic: Partial<WebuiSettings["public"]> = {...configuration.public};
    const legacyServer: Partial<WebuiSettings["server"]> = {...configuration.server};
    delete legacyPublic.S3ScannerEnabled;
    delete legacyServer.LogIngestorUrl;
    const parsed = Value.Parse(WebuiSettingsSchema, {public: legacyPublic, server: legacyServer});
    t.equal(parsed.public.S3ScannerEnabled, false);
    t.equal(parsed.server.LogIngestorUrl, null);
    t.notOk(isS3ScannerEnabled(parsed.public));
    t.end();
});
