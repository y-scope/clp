import {
    useMutation,
    useQueryClient,
} from "@tanstack/react-query";
import {
    CompressionJob,
    CompressionJobCreation,
} from "@webui/common/schemas/compression";
import axios, {AxiosResponse} from "axios";


/**
 * Submits a compression job.
 *
 * @param payload
 * @return
 */
const submitCompressionJob = async (payload: CompressionJob)
: Promise<AxiosResponse<CompressionJobCreation>> => {
    console.log("Submitting compression job:", JSON.stringify(payload));

    return await axios.post("/api/compress", payload);
};

/**
 * Submits compression jobs with react-query mutation.
 *
 * @return a react-query mutation hook.
 */
const useSubmitCompressionJob = () => {
    const queryClient = useQueryClient();

    return useMutation({
        mutationFn: submitCompressionJob,
        onSettled: async () => {
            // Invalidate queries that are affected by a new compression job.
            await queryClient.invalidateQueries({queryKey: ["jobs"]});
        },
    });
};


export {
    submitCompressionJob,
    useSubmitCompressionJob,
};
