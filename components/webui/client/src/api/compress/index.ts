import type {
    CompressionJobCreationSchema,
    CompressionJobSchema,
} from "@webui/common/compression";
import axios from "axios";


/**
 * Submits a compression job.
 *
 * @param payload
 * @return
 */
const submitCompressionJob = async (payload: CompressionJobSchema): Promise<number> => {
    console.log("Submitting compression job:", JSON.stringify(payload));

    const response = await axios.post<CompressionJobCreationSchema>("/api/compress", payload);
    return response.data.jobId;
};

export type {
    CompressionJobCreationSchema,
    CompressionJobSchema,
};
export {submitCompressionJob};
