import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {Type} from "@sinclair/typebox";
import {FileListingSchema} from "@webui/common/schemas/os";
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

                const errMsg = "Unable to list files";
                request.log.error(e, errMsg);

                return reply.internalServerError(errMsg);
            }
        }
    );
};

export default plugin;
