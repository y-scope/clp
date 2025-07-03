import {StatusCodes} from "http-status-codes";
import tap, {Test} from "tap";

import {build} from "./tap.js";


tap.test("Tests the example routes", async (t: Test) => {
    const server = await build(t);

    let resp = await server.inject({
        method: "GET",
        url: "/example/get/Alice",
    });

    t.equal(resp.statusCode, StatusCodes.OK);
    t.match(JSON.parse(resp.body), {msg: String});

    resp = await server.inject({
        method: "POST",
        url: "/example/post",
        payload: {name: "Bob"},
    });
    t.equal(resp.statusCode, StatusCodes.OK);
    t.match(JSON.parse(resp.body), {msg: String});
});

// eslint-disable-next-line no-warning-comments
// TODO: Add tests for `query` routes.
