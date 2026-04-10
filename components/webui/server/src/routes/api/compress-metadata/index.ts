import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {JobsResponseSchema} from "@webui/common/schemas/compress-metadata";
import {constants} from "http2";

import settings from "../../../../settings.json" with {type: "json"};
import {
    CompressionMetadataQueryRow,
    getCompressionMetadataQuery,
    getIngestionJobsQuery,
    IngestionJobQueryRow,
} from "./sql.js";
import {
    mapCompressionMetadataRows,
    mapIngestionJobRows,
} from "./utils.js";


/**
 * Compression metadata route.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    fastify.get(
        "/",
        {
            schema: {
                response: {
                    [constants.HTTP_STATUS_OK]: JobsResponseSchema,
                },
                tags: ["Compression Metadata"],
            },
        },
        async () => {
            const [compressionRows] =
                await fastify.mysql.query<CompressionMetadataQueryRow[]>(
                    getCompressionMetadataQuery()
                );

            const compressionJobs = mapCompressionMetadataRows(compressionRows);

            const hasLogIngestor = null !== (settings.LogIngestorHost as string | null);
            if (false === hasLogIngestor) {
                return {compressionJobs: compressionJobs, ingestionJobs: []};
            }

            const [ingestionRows] =
                await fastify.mysql.query<IngestionJobQueryRow[]>(
                    getIngestionJobsQuery()
                );

            return {
                compressionJobs: compressionJobs,
                ingestionJobs: mapIngestionJobRows(ingestionRows),
            };
        }
    );
};

export default plugin;
