import assert from "node:assert/strict";
import test from "node:test";

import {parseCompressionJobs} from "../src/api/compress-metadata/parse.ts";


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
    duration: null,
    start_time: null,
    status: 2,
    status_msg: "",
    uncompressed_size: 10,
    update_time: "2026-10-15T00:00:00Z",
};

test("accepts partial compression configs from older jobs", () => {
    const [row] = parseCompressionJobs([job]);
    assert.deepEqual(row.clp_config.input.paths_to_compress, ["/mnt/logs/app.log"]);
});

test("keeps stored S3 authentication out of the display row", () => {
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
    const malformed = {
        ...job,
        clp_config: {input: "not an input object", output: {}},
    };

    assert.throws(
        () => parseCompressionJobs([malformed]),
        /Compression history contains invalid metadata/
    );
});
