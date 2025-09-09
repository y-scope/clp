import {
    Static,
    Type,
} from "@sinclair/typebox";
import axios, {AxiosResponse} from "axios";


// eslint-disable-next-line @typescript-eslint/no-unused-vars
const FileListResponseSchema = Type.Array(
    Type.Object({
        isExpandable: Type.Boolean(),
        name: Type.String(),
        parentPath: Type.String(),
    })
);


type FileListResponseSchemaType = Static<typeof FileListResponseSchema>;


/**
 * Lists files and directories at the specified path.
 *
 * @param path The path to list files for
 * @return
 */
const listFiles = async (path: string): Promise<AxiosResponse<FileListResponseSchemaType>> => {
    return axios.get<FileListResponseSchemaType>(`/api/os/ls?path=${encodeURIComponent(path)}`);
};

export {listFiles};
