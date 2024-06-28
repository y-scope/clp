import httpStatusCodes from "http-status-codes";
import {test} from "tap";

import app from "./app.js";
import {parseEnvVars} from "./utils.js";


test("Tests the example routes", async (t) => {
    const envVars = parseEnvVars();
    const server = await app(
        {
            fastifyOptions: {
                logger: false,
            },
            dbUser: envVars.CLP_DB_USER,
            dbPass: envVars.CLP_DB_PASS,
        },
    );
    t.teardown(() => server.close());

    let resp = await server.inject({
        method: "POST",
        url: "/decompression_job",
        payload: {
            jobId: 1,
            status: "pending",
        },
    });

    t.equal(resp.statusCode, httpStatusCodes.OK);

    resp = await server.inject({
        method: "GET",
        url: "/decompression_job/1",
    });
    t.equal(resp.statusCode, httpStatusCodes.OK);
    t.match(JSON.parse(resp.body), {
        jobId: 1,
        status: "pending",
    });

    resp = await server.inject({
        method: "GET",
        url: "/stats",
    });
    t.equal(resp.statusCode, httpStatusCodes.OK);
    console.log(JSON.parse(resp.body));
    t.match(JSON.parse(resp.body), []);
});
