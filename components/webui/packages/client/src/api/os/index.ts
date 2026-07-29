import {FileListing} from "@webui/common/schemas/os";
import axios from "axios";


/**
 * Lists files and directories at the specified path.
 *
 * @param path
 * @return
 */
const listFiles = async (path: string): Promise<FileListing> => {
    const {data} = await axios.get<FileListing>("/api/os/ls", {
        params: {path},
    });

    return data;
};

export {listFiles};
