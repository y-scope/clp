const fastifyPlugin = require("fastify-plugin");


module.exports = fastifyPlugin(async (app) => {
    // Demonstrates how to R/W data with the DbManger
    app.get("/mongo/stats", async (request, reply) => {
        const result = await app.dbManager.getStatsFromMongo();
        reply.send(result);
    });

    app.get("/mysql/search_jobs", async (request, reply) => {
        const result = await app.dbManager.getSearchJobsFromMySQL();
        reply.send(result);
    });

    app.post("/mysql/search_jobs", async (request, reply) => {
        const result = await app.dbManager.createSearchJobInMySQL(request.body);
        reply.send(result);
    });
});
