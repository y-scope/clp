import assert from "node:assert/strict";
import test from "node:test";

import {createApiError} from "../src/api/utils.ts";


test("shows an API validation message", () => {
    const response = new Response("Invalid dataset name", {
        status: 400,
        headers: {"content-type": "text/plain; charset=utf-8"},
    });

    assert.equal(
        createApiError("Failed to fetch time range", response, "Invalid dataset name").message,
        "Failed to fetch time range: Invalid dataset name"
    );
});

test("shows a plain-text request body rejection", () => {
    const message = "Failed to deserialize the JSON body into the target type: unknown field `x`";
    const response = new Response(message, {
        status: 422,
        headers: {"content-type": "text/plain; charset=utf-8"},
    });

    assert.equal(
        createApiError("Failed to submit compression job", response, message).message,
        `Failed to submit compression job: ${message}`
    );
});

test("shows a not-found message for a bodyless 404", () => {
    const response = new Response(null, {status: 404});

    assert.equal(
        createApiError("Failed to fetch time range", response, null).message,
        "Failed to fetch time range: Not found."
    );
});

test("does not show internal API response details", () => {
    const response = new Response("database connection failed", {status: 500});

    assert.equal(
        createApiError("Failed to fetch details", response, "database connection failed").message,
        "Failed to fetch details: HTTP 500"
    );
});

test("gives a safe retry message after extraction times out", () => {
    const response = new Response("private diagnostic", {status: 504});

    assert.equal(
        createApiError("Failed to extract stream file", response, "private diagnostic").message,
        "Failed to extract stream file: Request timed out. Try again later."
    );
});
