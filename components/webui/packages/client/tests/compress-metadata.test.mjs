import assert from "node:assert/strict";
import test from "node:test";

import {parseCompressionJobs} from "../src/api/compress-metadata/parse.ts";


const TARGET_ARCHIVE_SIZE = 268435456;

const job = {
    _id: 42,
    clp_config: {
        input: {
            paths_to_compress: ["/mnt/logs/app.log"],
            type: "fs",
        },
        output: {},
    },
    compressed_size: 5,
    duration: 1.5,
    start_time: "2026-10-15T00:00:00Z",
    status: 2,
    status_msg: "",
    uncompressed_size: 10,
    update_time: "2026-10-15T00:00:01Z",
};

test("accepts display configs with omitted fields", () => {
    const [row] = parseCompressionJobs([job]);
    assert.deepEqual(row.clp_config.input.paths_to_compress, ["/mnt/logs/app.log"]);
    assert.equal("dataset" in row.clp_config.input, false);
});

test("accepts empty config sections from older jobs", () => {
    const olderJob = {
        ...job,
        clp_config: {input: {}, output: {}},
        duration: null,
        start_time: null,
    };
    const [row] = parseCompressionJobs([olderJob]);

    assert.deepEqual(row.clp_config, {input: {}, output: {}});
});

test("accepts integer config sizes", () => {
    const [row] = parseCompressionJobs([{
        ...job,
        clp_config: {
            input: job.clp_config.input,
            output: {target_archive_size: TARGET_ARCHIVE_SIZE},
        },
    }]);

    assert.equal(row.clp_config.output.target_archive_size, TARGET_ARCHIVE_SIZE);
});

test("keeps fields outside the display contract out of the display row", () => {
    const s3Job = {
        ...job,
        clp_config: {
            input: {
                aws_authentication: {credentials: {secret_access_key: "synthetic-only"}},
                keys: ["logs/app.log"],
                type: "s3",
            },
            output: {},
        },
    };
    const [row] = parseCompressionJobs([s3Job]);

    assert.equal("aws_authentication" in row.clp_config.input, false);
});

test("rejects malformed compression configs before the Jobs table uses them", () => {
    for (const clpConfig of [
        {input: "not an input object", output: {}},
        {output: {}},
        {input: {}, output: {target_archive_size: "large"}},
    ]) {
        assert.throws(
            () => parseCompressionJobs([{...job, clp_config: clpConfig}]),
            /Compression history contains invalid metadata/
        );
    }
});
