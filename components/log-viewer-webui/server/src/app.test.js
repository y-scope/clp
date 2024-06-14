import httpStatusCodes from "http-status-codes";
import {test} from "tap";

import app from "./app.js";


test("Tests the example routes", async (t) => {
    const server = await app();
    t.teardown(() => server.close());

    let resp = await server.inject({
        method: "GET",
        url: "/examples/get/Alice",
    });

    t.equal(resp.statusCode, httpStatusCodes.OK);
    t.match(JSON.parse(resp.body), {msg: String});

    resp = await server.inject({
        method: "POST",
        url: "/examples/post",
        payload: {name: "Bob"},
    });
    t.equal(resp.statusCode, httpStatusCodes.OK);
    t.match(JSON.parse(resp.body), {msg: String});
});
