import {JobsResponse} from "@webui/common/schemas/compress-metadata";
import axios from "axios";


/**
 * Retrieves recent compression and ingestion jobs.
 *
 * @return Jobs response containing compression and ingestion job metadata.
 */
const fetchJobs = async (): Promise<JobsResponse> => {
    const {data} = await axios.get<JobsResponse>(
        "/api/compress-metadata"
    );

    return data;
};


export {fetchJobs};
