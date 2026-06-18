import {CompressionMetadataDecoded} from "@webui/common/schemas/compress-metadata";
import axios from "axios";


/**
 * Retrieves recent compression jobs (last 30 days).
 *
 * @return Recent compression jobs metadata.
 */
const fetchCompressionJobs = async (): Promise<CompressionMetadataDecoded[]> => {
    const {data} = await axios.get<CompressionMetadataDecoded[]>(
        "/api/compress-metadata"
    );

    return data;
};


export {fetchCompressionJobs};
