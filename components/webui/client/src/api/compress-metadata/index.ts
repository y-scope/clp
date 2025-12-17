import {CompressionJobWithDecodedIoConfig} from "@webui/common/schemas/compress-metadata";
import axios from "axios";


/**
 * Retrieves compression jobs updated after the given timestamp.
 *
 * @param lastUpdateTimestampSeconds
 * @return
 */
const fetchCompressionJobs = async (
    lastUpdateTimestampSeconds: number
): Promise<CompressionJobWithDecodedIoConfig[]> => {
    const {data} = await axios.get<CompressionJobWithDecodedIoConfig[]>(
        "/api/compress-metadata/jobs",
        {params: {lastUpdateTimestampSeconds}}
    );

    return data;
};


export {fetchCompressionJobs};
