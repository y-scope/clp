import {CLP_DEFAULT_DATASET_NAME} from "@webui/common/config";
import type {
    S3ScannerConfig,
    S3ScannerCreation,
    S3ScannerJob,
} from "@webui/common/schemas/ingestion";
import axios, {isAxiosError} from "axios";


/**
 * Submits one scanner request without retrying an uncertain creation outcome.
 *
 * @param payload
 * @return The created scanner's ID.
 */
const submitS3Scanner = async (payload: S3ScannerCreation): Promise<S3ScannerJob> => {
    const config: S3ScannerConfig = {
        bucket_name: payload.bucket,
        dataset: payload.dataset || CLP_DEFAULT_DATASET_NAME,
        key_prefix: payload.keyPrefix,
        region: payload.region,
        ...(true !== payload.unstructured && payload.timestampKey ?
            {timestamp_key: payload.timestampKey} :
            {}),
        unstructured: payload.unstructured ?? false,
    };

    try {
        const {data} = await axios.post<S3ScannerJob>("/api/log-ingestor/s3_scanner", config);

        return data;
    } catch (error: unknown) {
        if (isAxiosError<{message?: string}>(error) && error.response?.data.message) {
            throw new Error(error.response.data.message);
        }

        throw new Error("Could not confirm scanner creation. Check the ingestor before retrying.");
    }
};


export {submitS3Scanner};
