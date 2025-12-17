import {CompressionMetadataDecoded} from "@webui/common/schemas/compress-metadata";
import axios from "axios";


/**
 * Retrieves compression jobs updated after the given timestamp.
 *
 * @param lastUpdateTimestampSeconds
 * @return
 */
const fetchCompressionJobs = async (
    lastUpdateTimestampSeconds: number
): Promise<CompressionMetadataDecoded[]> => {
    const {data} = await axios.get<CompressionMetadataDecoded[]>(
        "/api/compress-metadata/jobs",
        {params: {lastUpdateTimestampSeconds}}
    );

    return data;
};


export {fetchCompressionJobs};
