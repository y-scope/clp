/**
 * Builds the message for a failed API-server request, preferring the body the API server sent
 * over its bare status code.
 *
 * The generated client returns the raw response text as `error` (or the parsed JSON when the body
 * happens to parse). The API server sends a plain-text explanation for 400 and 504 responses, and
 * an empty body for 404 and 500, in which case only the status code is available.
 *
 * @param context Short description of what failed, e.g. "Failed to list files".
 * @param error The `error` field returned by the generated client.
 * @param response The `response` field returned by the generated client.
 * @return The message to put in the thrown `Error`.
 */
const buildApiErrorMessage = (context: string, error: unknown, response: Response): string => {
    let detail = "";
    if ("string" === typeof error) {
        detail = error.trim();
    } else if (null !== error && "object" === typeof error && "message" in error &&
        "string" === typeof (error).message) {
        detail = (error as {message: string}).message.trim();
    }

    return "" === detail ?
        `${context}: HTTP ${response.status}` :
        `${context}: ${detail} (HTTP ${response.status})`;
};

export {buildApiErrorMessage};
