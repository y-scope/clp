import {
    FastifyPluginAsyncTypebox,
    Type,
} from "@fastify/type-provider-typebox";
import {
    CompressionMetadataDecoded,
    CompressionMetadataDecodedSchema,
} from "@webui/common/schemas/compress-metadata";
import {constants} from "http2";

import {mapCompressionMetadataRows} from "./utils.js";
import {CompressionMetadataQueryRow, getCompressionMetadataQuery} from "./sql.js";

/**
 * Compression metadata - jobs routes.
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
            const [rows] = await fastify.mysql.query<CompressionMetadataQueryRow[]>(
                getCompressionMetadataQuery()
            );

            return mapCompressionMetadataRows(rows);
        }
    );
};

export default plugin;
