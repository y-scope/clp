import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {Type} from "@sinclair/typebox";
import {FileListingSchema} from "@webui/common/schemas/os";
import fs from "fs/promises";
import {constants} from "http2";


const FileListRequestSchema = Type.Object({
    path: Type.String({
        default: "/",
    }),
});


/**
 * File listing API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    /**
     * Lists files and directories at the specified path.
     */
    fastify.get(
        "/ls",
        {
            schema: {
                querystring: FileListRequestSchema,
                response: {
                    [constants.HTTP_STATUS_OK]: FileListingSchema,
                },
            },
        },
        async (request, reply) => {
            const {path: requestedPath} = request.query;

            try {
                await fs.access(requestedPath);
            } catch {
                return await reply.notFound(`Path not found: ${requestedPath}`);
            }

            try {
                const direntList = await fs.readdir(requestedPath, {withFileTypes: true});
                return direntList.map((dirent) => {
                    const isExpandable = dirent.isDirectory() || dirent.isSymbolicLink();
                    return {
                        isExpandable: isExpandable,
                        name: dirent.name,
                        parentPath: dirent.parentPath,
                    };
                });
            } catch (e: unknown) {
                if (reply.sent) {
                    return Promise.resolve();
                }

                const errMsg = "Unable to list files";
                request.log.error(e, errMsg);

                return reply.internalServerError(errMsg);
            }
        }
    );
};

export default plugin;
