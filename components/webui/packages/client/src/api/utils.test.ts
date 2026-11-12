import {
    describe,
    expect,
    it,
} from "vitest";

import {createApiError} from "./utils";


describe("createApiError", () => {
    it("shows an API validation message", () => {
        const response = new Response("Invalid dataset name", {
            status: 400,
            headers: {"content-type": "text/plain; charset=utf-8"},
        });

        expect(
            createApiError("Failed to fetch time range", response, "Invalid dataset name").message,
        ).toBe("Failed to fetch time range: Invalid dataset name");
    });

    it("shows a plain-text request body rejection", () => {
        const message =
            "Failed to deserialize the JSON body into the target type: unknown field `x`";
        const response = new Response(message, {
            status: 422,
            headers: {"content-type": "text/plain; charset=utf-8"},
        });

        expect(
            createApiError("Failed to submit compression job", response, message).message,
        ).toBe(`Failed to submit compression job: ${message}`);
    });

    it("shows a not-found message for a bodyless 404", () => {
        const response = new Response(null, {status: 404});

        expect(
            createApiError("Failed to fetch time range", response, null).message,
        ).toBe("Failed to fetch time range: Not found.");
    });

    it("does not show internal API response details", () => {
        const response = new Response("database connection failed", {status: 500});

        expect(
            createApiError("Failed to fetch details", response, "database connection failed")
                .message,
        ).toBe("Failed to fetch details: HTTP 500");
    });

    it("gives a safe retry message after extraction times out", () => {
        const response = new Response("private diagnostic", {status: 504});

        expect(
            createApiError("Failed to extract stream file", response, "private diagnostic")
                .message,
        ).toBe("Failed to extract stream file: Request timed out. Try again later.");
    });
});
