import httpStatusCodes from "http-status-codes";
import {test} from "tap";

import app from "./app.js";


test("Tests the example routes", async (t) => {
    const server = await app({});
    t.teardown(() => server.close());

    let resp = await server.inject({
        method: "GET",
        url: "/example/get/Alice",
    });

    t.equal(resp.statusCode, httpStatusCodes.OK);
    t.match(JSON.parse(resp.body), {msg: String});

    resp = await server.inject({
        method: "POST",
        url: "/example/post",
        payload: {name: "Bob"},
    });
    t.equal(resp.statusCode, httpStatusCodes.OK);
    t.match(JSON.parse(resp.body), {msg: String});
});

// eslint-disable-next-line no-warning-comments
// TODO: Add tests for `query` routes.
