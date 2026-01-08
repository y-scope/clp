import {FastifyPluginAsyncTypebox} from "@fastify/type-provider-typebox";
import {
    ComponentsListingSchema,
    ListComponentsRequestSchema,
    ListLogFilesRequestSchema,
    LogContentResponseSchema,
    LogFilesListingSchema,
    ReadLogContentRequestSchema,
} from "@webui/common/schemas/operational-logs";
import fs from "fs/promises";
import {constants} from "http2";
import path from "path";

import settings from "../../../../settings.json" with {type: "json"};


/**
 * Parses a Fluent Bit log line.
 *
 * Format: `{tag}: [{timestamp}, {json_data}]`
 *
 * @param line The raw log line
 * @return Parsed log entry or null if parsing fails
 */
const parseFluentBitLogLine = (line: string): {
    timestamp: number;
    source: string;
    log: string;
    containerId?: string;
    containerName?: string;
    clpDeployment?: string;
} | null => {
    // Match pattern: tag: [timestamp, {json}]
    const match = line.match(/^[^:]+:\s*\[(\d+(?:\.\d+)?),\s*(.+)\]$/);
    if (null === match) {
        // Try to parse as plain text log line
        return {
            timestamp: Date.now() / 1000,
            source: "stdout",
            log: line,
        };
    }

    const timestamp = parseFloat(match[1] ?? "0");
    const jsonStr = match[2];

    if ("undefined" === typeof jsonStr) {
        return null;
    }

    try {
        const data = JSON.parse(jsonStr) as {
            source?: string;
            log?: string;
            container_id?: string;
            container_name?: string;
            clp_deployment?: string;
        };

        const result: {
            timestamp: number;
            source: string;
            log: string;
            containerId?: string;
            containerName?: string;
            clpDeployment?: string;
        } = {
            timestamp: timestamp,
            source: data.source ?? "stdout",
            log: data.log ?? "",
        };

        if ("undefined" !== typeof data.container_id) {
            result.containerId = data.container_id;
        }
        if ("undefined" !== typeof data.container_name) {
            result.containerName = data.container_name;
        }
        if ("undefined" !== typeof data.clp_deployment) {
            result.clpDeployment = data.clp_deployment;
        }

        return result;
    } catch {
        return null;
    }
};

/**
 * Operational logs API routes.
 *
 * @param fastify
 */
const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const operationalLogsDir = (settings as {OperationalLogsDir?: string}).OperationalLogsDir ??
        "/var/log/clp";

    /**
     * Lists available components with logs.
     */
    fastify.get(
        "/components",
        {
            schema: {
                querystring: ListComponentsRequestSchema,
                response: {
                    [constants.HTTP_STATUS_OK]: ComponentsListingSchema,
                },
                tags: ["Operational Logs"],
            },
        },
        async (request, reply) => {
            try {
                await fs.access(operationalLogsDir);
            } catch {
                return await reply.notFound(`Operational logs directory not found: ${operationalLogsDir}`);
            }

            try {
                const entries = await fs.readdir(operationalLogsDir, {withFileTypes: true});
                const components = await Promise.all(
                    entries
                        .filter((entry) => entry.isDirectory())
                        .map(async (entry) => {
                            const componentDir = path.join(operationalLogsDir, entry.name);
                            const files = await fs.readdir(componentDir);
                            return {
                                name: entry.name,
                                hasLogs: files.length > 0,
                            };
                        })
                );

                return components.sort((a, b) => a.name.localeCompare(b.name));
            } catch (e: unknown) {
                const errMsg = "Unable to list components";
                request.log.error(e, errMsg);

                return reply.internalServerError(errMsg);
            }
        }
    );

    /**
     * Lists log files for a component or all components.
     */
    fastify.get(
        "/files",
        {
            schema: {
                querystring: ListLogFilesRequestSchema,
                response: {
                    [constants.HTTP_STATUS_OK]: LogFilesListingSchema,
                },
                tags: ["Operational Logs"],
            },
        },
        async (request, reply) => {
            const {component} = request.query;

            try {
                await fs.access(operationalLogsDir);
            } catch {
                return await reply.notFound(`Operational logs directory not found: ${operationalLogsDir}`);
            }

            try {
                const logFiles: Array<{
                    component: string;
                    filename: string;
                    path: string;
                    sizeBytes: number;
                    modifiedTime: number;
                }> = [];

                // Get list of component directories to scan
                let componentDirs: string[];
                if ("undefined" !== typeof component && "" !== component) {
                    componentDirs = [component];
                } else {
                    const entries = await fs.readdir(operationalLogsDir, {withFileTypes: true});
                    componentDirs = entries
                        .filter((entry) => entry.isDirectory())
                        .map((entry) => entry.name);
                }

                // Scan each component directory for log files
                for (const componentName of componentDirs) {
                    const componentDir = path.join(operationalLogsDir, componentName);

                    try {
                        const files = await fs.readdir(componentDir);
                        for (const filename of files) {
                            const filePath = path.join(componentDir, filename);
                            const stats = await fs.stat(filePath);

                            if (stats.isFile()) {
                                logFiles.push({
                                    component: componentName,
                                    filename: filename,
                                    path: filePath,
                                    sizeBytes: stats.size,
                                    modifiedTime: stats.mtimeMs,
                                });
                            }
                        }
                    } catch {
                        // Skip directories that don't exist or can't be read
                    }
                }

                // Sort by modified time descending (newest first)
                logFiles.sort((a, b) => b.modifiedTime - a.modifiedTime);

                return logFiles;
            } catch (e: unknown) {
                const errMsg = "Unable to list log files";
                request.log.error(e, errMsg);

                return reply.internalServerError(errMsg);
            }
        }
    );

    /**
     * Reads and parses log content from a file.
     */
    fastify.get(
        "/content",
        {
            schema: {
                querystring: ReadLogContentRequestSchema,
                response: {
                    [constants.HTTP_STATUS_OK]: LogContentResponseSchema,
                },
                tags: ["Operational Logs"],
            },
        },
        async (request, reply) => {
            const {path: requestedPath, offset = 0, limit = 100} = request.query;

            // Security: Ensure path is within operational logs directory
            const normalizedPath = path.normalize(requestedPath);
            if (false === normalizedPath.startsWith(operationalLogsDir)) {
                return await reply.badRequest("Invalid log file path");
            }

            try {
                await fs.access(normalizedPath);
            } catch {
                return await reply.notFound(`Log file not found: ${normalizedPath}`);
            }

            try {
                const content = await fs.readFile(normalizedPath, "utf-8");
                const lines = content.split("\n").filter((line) => line.trim().length > 0);
                const totalLines = lines.length;

                // Apply offset and limit
                const selectedLines = lines.slice(offset, offset + limit);
                const entries = selectedLines
                    .map(parseFluentBitLogLine)
                    .filter((entry): entry is NonNullable<typeof entry> => null !== entry);

                return {
                    entries: entries,
                    totalLines: totalLines,
                    hasMore: offset + limit < totalLines,
                };
            } catch (e: unknown) {
                const errMsg = "Unable to read log file";
                request.log.error(e, errMsg);

                return reply.internalServerError(errMsg);
            }
        }
    );
};

export default plugin;
