import {StatusCodes} from "http-status-codes";
import tap from "tap";
import fastify from 'fastify'
import serviceApp from '../../src/app.js'
import fp from 'fastify-plugin'

import app from "./app.js";


tap.test("Tests the example routes", async (t) => {
    const app = fastify()
    await app.register(fp(serviceApp))
    const server = await app({fastifyOptions: {}, sqlDbPass: "", sqlDbUser: ""});
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
