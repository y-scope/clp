import {zstdDecompressSync} from "node:zlib";

import type {MySQLPromisePool} from "@fastify/mysql";
import {decode} from "@msgpack/msgpack";
import {
    type ClpIoConfig,
    CompressionJobInputType,
} from "@webui/common/schemas/compression";
import fastify from "fastify";
import type {
    FieldPacket,
    ResultSetHeader,
} from "mysql2";
import tap, {type Test} from "tap";


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

const INSERT_ID = 7;

tap.test(
    "CompressionJobDbManager stores zstd-compressed MessagePack job config",
    async (t: Test) => {
        const {default: compressionJobDbManagerPlugin} = await t.mockImport<
            typeof import("../plugins/app/CompressionJobDbManager/index.js")
        >(
            "../plugins/app/CompressionJobDbManager/index.js",
            {
                "../settings.js": {
                    publicSettings: {
                        SqlDbCompressionJobsTableName: "compression_jobs",
                    },
                },
            }
        );
        let queryValues: unknown[] | undefined;
        const sqlPool = {
            query: async (_query: unknown, values?: unknown[]) => {
                queryValues = values;

                return [
                    {
                        insertId: INSERT_ID,
                    } as ResultSetHeader,
                    [] as FieldPacket[],
                ];
            },
        } as unknown as MySQLPromisePool;
        const server = fastify();
        server.decorate("mysql", sqlPool);
        server.register(compressionJobDbManagerPlugin);
        t.teardown(async () => {
            await server.close();
        });
        await server.ready();

        const jobId = await server.CompressionJobDbManager.submitJob(CLP_IO_CONFIG);

        t.equal(jobId, INSERT_ID);
        t.ok(Array.isArray(queryValues));
        const [jobConfig] = queryValues as [Buffer];
        t.ok(Buffer.isBuffer(jobConfig));
        t.same(decode(zstdDecompressSync(jobConfig)), CLP_IO_CONFIG);
    }
);
