import createClient, {type ClientOptions} from "openapi-fetch";

import type {
    components,
    paths,
} from "./schema.js";


/**
 * Creates a type-safe client for the CLP API server.
 *
 * @param clientOptions
 * @return
 */
const createApiClient = (clientOptions?: ClientOptions) => createClient<paths>(clientOptions);

type ApiClient = ReturnType<typeof createApiClient>;

export type {
    ApiClient,
    components,
    paths,
};
export {createApiClient};
