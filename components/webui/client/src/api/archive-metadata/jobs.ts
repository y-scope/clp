import axios from "axios";

interface CompressionJobApiItem {
    _id: number;
    compressed_size: number;
    dataset: string | null;
    duration: number | null;
    paths: string[];
    retrieval_time: number;
    start_time: string | null;
    status: number;
    status_msg: string;
    uncompressed_size: number;
    update_time: string;
}

/**
 * Fetch compression jobs from the server with decoded config fields.
 *
 * @param lastUpdateTimestampSeconds
 * @return
 */
const fetchCompressionJobs = async (
    lastUpdateTimestampSeconds: number
): Promise<CompressionJobApiItem[]> => {
    const {data} = await axios.get<CompressionJobApiItem[]>(
        "/api/archive-metadata/jobs",
        {
            params: {lastUpdateTimestampSeconds},
        }
    );

    return data;
};


export type {CompressionJobApiItem};
export {fetchCompressionJobs};
