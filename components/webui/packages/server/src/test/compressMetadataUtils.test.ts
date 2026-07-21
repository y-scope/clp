import {zstdCompressSync} from "node:zlib";

import {encode} from "@msgpack/msgpack";
import {
    type ClpIoConfig,
    CompressionJobInputType,
} from "@webui/common/schemas/compression";
import tap, {type Test} from "tap";

import type {CompressionMetadataQueryRow} from "../routes/api/compress-metadata/sql.js";
import {
    decodeJobConfig,
    mapCompressionMetadataRows,
} from "../routes/api/compress-metadata/utils.js";


const CLP_IO_CONFIG: ClpIoConfig = {
    input: {
        dataset: null,
        path_prefix_to_remove: "/mnt/logs",
        paths_to_compress: [
            "/mnt/logs/app.log",
        ],
        timestamp_key: null,
        type: CompressionJobInputType.FS,
        unstructured: true,
    },
    output: {
        compression_level: 3,
        target_archive_size: 1073741824,
        target_dictionaries_size: 268435456,
        target_encoded_file_size: 268435456,
        target_segment_size: 268435456,
    },
};

/**
 * Creates a zstd-compressed MessagePack copy of the sample CLP IO config.
 *
 * @return Serialized bytes ready for database storage.
 */
const createCompressedJobConfig = (): Buffer => {
    return Buffer.from(zstdCompressSync(encode(CLP_IO_CONFIG)));
};

tap.test("decodeJobConfig decodes zstd-compressed MessagePack job config", (t: Test) => {
    t.same(decodeJobConfig(createCompressedJobConfig()), {clp_config: CLP_IO_CONFIG});
    t.end();
});

tap.test("decodeJobConfig rejects missing job config buffers", (t: Test) => {
    t.throws(
        () => decodeJobConfig(null),
        {message: "Missing clp_config buffer for compression metadata"}
    );
    t.end();
});

tap.test("decodeJobConfig rejects invalid zstd payloads", (t: Test) => {
    const originalConsoleError = console.error;
    console.error = () => null;
    t.teardown(() => {
        console.error = originalConsoleError;
    });

    t.throws(
        () => decodeJobConfig(Buffer.from("not-zstd")),
        {message: "Failed to decode clp_config buffer"}
    );
    t.end();
});

tap.test("mapCompressionMetadataRows decodes row job configs", (t: Test) => {
    const row = {
        _id: 1,
        clp_config: createCompressedJobConfig(),
        compressed_size: 2,
        duration: 3,
        start_time: "2026-07-07 00:00:00",
        status: 4,
        status_msg: "done",
        uncompressed_size: 5,
        update_time: "2026-07-07 00:00:01",
    } as CompressionMetadataQueryRow;

    t.same(mapCompressionMetadataRows([row]), [
        {
            _id: row._id,
            clp_config: CLP_IO_CONFIG,
            compressed_size: row.compressed_size,
            duration: row.duration,
            start_time: row.start_time,
            status: row.status,
            status_msg: row.status_msg,
            uncompressed_size: row.uncompressed_size,
            update_time: row.update_time,
        },
    ]);
    t.end();
});
