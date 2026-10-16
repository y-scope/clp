import type {
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
    try {
        const {data} = await axios.post<S3ScannerJob>("/api/ingest/s3-scanner", payload);

        return data;
    } catch (error: unknown) {
        if (isAxiosError<{error?: string}>(error) && error.response?.data.error) {
            throw new Error(error.response.data.error);
        }

        throw new Error("Could not confirm scanner creation. Check the ingestor before retrying.");
    }
};


export {submitS3Scanner};
