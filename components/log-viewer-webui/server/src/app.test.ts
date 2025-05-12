import {StatusCodes} from "http-status-codes";
import tap from "tap";
import fastify from 'fastify'
import serviceApp from 'src/fastify-v2/app.js'
import fp from 'fastify-plugin'

tap.test("Tests the example routes", async (t) => {
    const server = fastify();
    await server.register(fp(serviceApp));
    t.teardown(() => server.close());

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
