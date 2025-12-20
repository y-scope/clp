import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {CompressionMetadataDecodedSchema} from "@webui/common/schemas/compress-metadata";
import {constants} from "http2";

import {
    CompressionMetadataQueryRow,
    getCompressionMetadataQuery,
} from "./sql.js";
import {mapCompressionMetadataRows} from "./utils.js";


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
                    [constants.HTTP_STATUS_OK]: Type.Array(
                        CompressionMetadataDecodedSchema
                    ),
                },
                tags: ["Compression Metadata"],
            },
        },
        async () => {
            console.log("Fetching compression metadata...");
            const [rows] = await fastify.mysql.query<CompressionMetadataQueryRow[]>(
                getCompressionMetadataQuery()
            );

            return mapCompressionMetadataRows(rows);
        }
    );
};

export default plugin;
