import type {
    CompressionJobCreationSchema,
    CompressionJobSchema,
} from "@webui/common/compression";
import axios from "axios";
import { useMutation, useQueryClient } from "@tanstack/react-query";


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

/**
 * Submits compression jobs with react-query mutation.
 */
const useSubmitCompressionJob = () => {
    const queryClient = useQueryClient();
    
    return useMutation({
        mutationFn: submitCompressionJob,
        onSettled: () => {
            // Invalidate queries that are affected by a new compression job.
            queryClient.invalidateQueries({ queryKey: ["jobs"] });
        },
    });
};

export type {
    CompressionJobCreationSchema,
    CompressionJobSchema,
};
export { submitCompressionJob, useSubmitCompressionJob };
