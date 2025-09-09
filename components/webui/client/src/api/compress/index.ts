import axios, {AxiosResponse} from "axios";


type CompressionJobCreationSchema = {
    paths: string[];
    dataset?: string;
    timestampKey?: string;
};

type CompressionJobSchema = {
    jobId: number;
};

/**
 * Submits a compression job.
 *
 * @param payload
 * @return
 */
const submitCompressionJob = async (payload: CompressionJobCreationSchema): Promise<number> => {
    console.log("Submitting compression job:", JSON.stringify(payload));

    const response = await axios.post<CompressionJobSchema>("/api/compress", payload);
    return response.data.jobId;
};

export type {
    CompressionJobCreationSchema,
    CompressionJobSchema,
};
export {
    submitCompressionJob,
};