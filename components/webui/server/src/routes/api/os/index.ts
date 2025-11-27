import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {Type} from "@sinclair/typebox";
import {StringSchema} from "@webui/common/schemas/common";
import fs from "fs/promises";
import {constants} from "http2";
import path from "path";

import settings from "../../../../settings.json" with {type: "json"};


/**
 * Resolves a requested path against the LsRoot setting.
 *
 * @param requestedPath
 * @return The resolved absolute path
 */
const resolveLsPath = (requestedPath: string): string => {
    let cleanPath = requestedPath;

    // Remove leading slashes for non-root LsRoot settings
    if ("/" !== settings.LsRoot) {
        cleanPath = cleanPath.replace(/^\/+/, "");
    }

    return path.resolve(settings.LsRoot, cleanPath);
};

/**
 * Normalizes a path for client display by removing the LsRoot prefix.
 *
 * @param fullPath
 * @return The normalized path relative to LsRoot
 */
const normalizeLsPath = (fullPath: string): string => {
    return fullPath.replace(new RegExp(`^${settings.LsRoot}/*`), "/");
};


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
                    [constants.HTTP_STATUS_OK]: FileListResponseSchema,
                },
            },
        },
        async (request, reply) => {
            const {path: requestedPath} = request.query;

            try {
                const resolvedPath = resolveLsPath(requestedPath);

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
                        parentPath: normalizeLsPath(dirent.parentPath),
                    };
                });
            } catch (e: unknown) {
                if (reply.sent) {
                    return Promise.resolve();
                }

                request.log.error(e, "Unable to list files");

                return reply.internalServerError(errMsg);
            }
        }
    );
};

export default plugin;
