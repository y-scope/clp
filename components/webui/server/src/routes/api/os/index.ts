import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {Type} from "@sinclair/typebox";
import fs from "fs/promises";
import {StatusCodes} from "http-status-codes";
import path from "path";

import settings from "../../../../settings.json" with {type: "json"};
import {StringSchema} from "../../../schemas/common.js";


const FileListRequestSchema = Type.Object({
    path: Type.String({
        default: "/",
    }),
});


const FileListResponseSchema = Type.Array(
    Type.Object({
        isExpandable: Type.Boolean(),
        name: StringSchema,
        parentPath: StringSchema,
    })
);


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
                    [StatusCodes.OK]: FileListResponseSchema,
                },
            },
        },
        async (request, reply) => {
            let {path: requestedPath} = request.query;

            try {
                let sliceLength = 0;
                if ("/" !== settings.LsRoot) {
                    (requestedPath = requestedPath.replace(/^\/+/, ""));
                    sliceLength = settings.LsRoot.length;
                }
                const resolvedPath = path.resolve(settings.LsRoot, requestedPath);

                try {
                    await fs.access(resolvedPath);
                } catch {
                    return await reply.notFound(`Path not found: ${resolvedPath}`);
                }

                const direntList = await fs.readdir(resolvedPath, {withFileTypes: true});
                return direntList.map((dirent) => {
                    const isExpandable = dirent.isDirectory() || dirent.isSymbolicLink();
                    return {
                        isExpandable: isExpandable,
                        name: dirent.name,
                        parentPath: dirent.parentPath.slice(sliceLength),
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
