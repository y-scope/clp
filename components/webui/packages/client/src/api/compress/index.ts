import {
    CompressionJob,
    CompressionJobCreation,
} from "@webui/common/schemas/compression";
import axios from "axios";


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


export {submitCompressionJob};
