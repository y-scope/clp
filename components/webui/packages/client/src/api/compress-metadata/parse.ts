import {Type} from "@sinclair/typebox";
import {Value} from "@sinclair/typebox/value";
import {
    type CompressionMetadata,
    CompressionMetadataSchema,
} from "@webui/common/schemas/compress-metadata";


const CompressionMetadataListSchema = Type.Array(CompressionMetadataSchema);

/**
 * Validates a compression-history response before the Jobs table reads its display configs.
 *
 * @param payload
 * @return Validated rows with only the display fields.
 * @throws {Error} If a row does not match the display contract.
 */
const parseCompressionJobs = (payload: unknown): CompressionMetadata[] => {
    try {
        return Value.Parse(CompressionMetadataListSchema, payload);
    } catch (error: unknown) {
        throw new Error("Compression history contains invalid metadata.", {cause: error});
    }
};

export {parseCompressionJobs};
