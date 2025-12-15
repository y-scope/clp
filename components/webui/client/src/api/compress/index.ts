import {
    CompressionJob,
    CompressionJobCreation,
} from "@webui/common/schemas/compression";
import axios from "axios";

interface CompressionJobWithConfig {
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
 * Submits a compression job.
 *
 * @param payload
 * @return
 */
const submitCompressionJob = async (payload: CompressionJobCreation): Promise<CompressionJob> => {
    console.log("Submitting compression job:", JSON.stringify(payload));
    const {data} = await axios.post<CompressionJob>("/api/compress", payload);

    return data;
};

/**
 * Fetches compression jobs with decoded dataset and paths.
 *
 * @param lastUpdateTimestampSeconds
 * @return
 */
const fetchCompressionJobs = async (lastUpdateTimestampSeconds: number) => {
    const {data} = await axios.get<CompressionJobWithConfig[]>(
        "/api/compress/jobs",
        {params: {lastUpdateTimestampSeconds}}
    );

    return data;
};


export {submitCompressionJob};
export type {CompressionJobWithConfig};
export {fetchCompressionJobs};
