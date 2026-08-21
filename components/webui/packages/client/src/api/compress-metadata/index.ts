import {Value} from "@sinclair/typebox/value";
import type {components} from "@webui/api-client";
import {
    type ClpIoConfig,
    ClpIoPartialConfigSchema,
} from "@webui/common/schemas/compression";

import {apiClient} from "../search";


type CompressionMetadata =
    Omit<components["schemas"]["CompressionMetadata"], "clp_config"> & {clp_config: ClpIoConfig};

/**
 * Validates the IO config the API server decoded for a compression job. The API server returns it
 * as an opaque JSON value, so it's checked here against the partial schema that tolerates configs
 * written by older CLP releases.
 *
 * @param clpConfig
 * @return
 * @throws {Error} If `clpConfig` doesn't match `ClpIoPartialConfigSchema`.
 */
const parseClpIoConfig = (clpConfig: unknown): ClpIoConfig => {
    try {
        return Value.Parse(ClpIoPartialConfigSchema, clpConfig);
    } catch (e: unknown) {
        throw new Error("Failed to parse a compression job's clp_config", {cause: e});
    }
};

/**
 * Retrieves recent compression jobs (last 30 days).
 *
 * @return Recent compression jobs metadata.
 * @throws {Error} If the request fails or the API server returns an unexpected response.
 */
const fetchCompressionJobs = async (): Promise<CompressionMetadata[]> => {
    // eslint-disable-next-line new-cap
    const {data, response} = await apiClient.GET("/metadata/compression_jobs", {});
    if ("undefined" === typeof data) {
        throw new Error(`Failed to fetch compression jobs: HTTP ${response.status}`);
    }

    return data.map((job) => ({
        ...job,
        clp_config: parseClpIoConfig(job.clp_config),
    }));
};

export {fetchCompressionJobs};
export type {CompressionMetadata};
