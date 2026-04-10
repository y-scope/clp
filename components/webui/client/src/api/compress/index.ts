import {
    CompressionJob,
    CompressionJobCreation,
    S3CompressionJobCreation,
    ScannerJobResponse,
} from "@webui/common/schemas/compression";
import axios from "axios";


/**
 * Submits a one-time compression job.
 *
 * @param payload
 * @return
 */
const submitCompressionJob = async (payload: CompressionJobCreation): Promise<CompressionJob> => {
    const {data} = await axios.post<CompressionJob>("/api/compress", payload);

    return data;
};

/**
 * Submits an S3 scanner job via the compress endpoint.
 *
 * @param payload
 * @return
 */
const submitScannerJob = async (
    payload: S3CompressionJobCreation
): Promise<ScannerJobResponse> => {
    const {data} = await axios.post<ScannerJobResponse>("/api/compress", payload);

    return data;
};


export {
    submitCompressionJob,
    submitScannerJob,
};
