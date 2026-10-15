import {Type} from "@sinclair/typebox";
import {Value} from "@sinclair/typebox/value";
import {
    type CompressionMetadataDecoded,
    CompressionMetadataDecodedSchema,
} from "@webui/common/schemas/compress-metadata";


const CompressionMetadataListSchema = Type.Array(CompressionMetadataDecodedSchema);

/**
 * Validates a compression-history response before the Jobs table reads legacy configs.
 *
 * @param payload
 * @return Validated rows with only the display fields.
 * @throws {Error} If a stored row does not match the display contract.
 */
const parseCompressionJobs = (payload: unknown): CompressionMetadataDecoded[] => {
    try {
        return Value.Parse(CompressionMetadataListSchema, payload);
    } catch (error: unknown) {
        throw new Error("Compression history contains invalid metadata.", {cause: error});
    }
};

export {parseCompressionJobs};
