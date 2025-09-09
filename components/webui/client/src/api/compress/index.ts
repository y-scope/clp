import axios from "axios";


type CompressionJobSchema = {
    paths: string[];
    dataset?: string;
    timestampKey?: string;
};

type CompressionJobCreationSchema = {
    jobId: number;
};

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
