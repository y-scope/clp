const HTTP_STATUS_BAD_REQUEST = 400;
const HTTP_STATUS_NOT_FOUND = 404;
const HTTP_STATUS_INTERNAL_SERVER_ERROR = 500;
const HTTP_STATUS_GATEWAY_TIMEOUT = 504;

/**
 * Converts a failed API response into a user-facing error.
 *
 * @param action
 * @param response
 * @param error
 * @return An error safe to display to the user.
 */
const createApiError = (action: string, response: Response, error: unknown): Error => {
    if (HTTP_STATUS_GATEWAY_TIMEOUT === response.status) {
        return new Error(`${action}: Request timed out. Try again later.`);
    }
    if (HTTP_STATUS_NOT_FOUND === response.status) {
        return new Error(`${action}: Not found.`);
    }
    const contentType = response.headers.get("content-type") ?? "";
    let detail: string | undefined;

    if (HTTP_STATUS_BAD_REQUEST <= response.status &&
        HTTP_STATUS_INTERNAL_SERVER_ERROR > response.status &&
        "string" === typeof error &&
        contentType.includes("text/plain")
    ) {
        detail = error.trim();
    }

    return new Error(detail ?
        `${action}: ${detail}` :
        `${action}: HTTP ${response.status}`);
};

export {createApiError};
