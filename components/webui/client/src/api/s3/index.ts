import {
    S3ListRequest,
    S3ListResponse,
} from "@webui/common/schemas/s3";
import axios from "axios";


/**
 * Lists S3 objects and common prefixes under the given bucket/prefix.
 *
 * @param params
 * @return
 */
const listS3Objects = async (params: S3ListRequest): Promise<S3ListResponse> => {
    const {data} = await axios.get<S3ListResponse>("/api/s3/ls", {params});

    return data;
};

export {listS3Objects};
