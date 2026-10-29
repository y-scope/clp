import fastify from "fastify";
import {constants} from "http2";
import tap, {Test} from "tap";

import routes from "../routes/api/example/index.js";


tap.test("Tests the example routes", async (t: Test) => {
    const server = fastify();
    server.register(routes);

    let resp = await server.inject({
        method: "GET",
        url: "/example/get/Alice",
    });

    t.equal(resp.statusCode, constants.HTTP_STATUS_OK);
    t.match(JSON.parse(resp.body), {msg: String});

    resp = await server.inject({
        method: "POST",
        url: "/example/post",
        payload: {name: "Bob"},
    });
    t.equal(resp.statusCode, constants.HTTP_STATUS_OK);
    t.match(JSON.parse(resp.body), {msg: String});
});
