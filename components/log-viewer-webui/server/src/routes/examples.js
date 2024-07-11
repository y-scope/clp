/**
 * Creates example routes.
 *
 * @param {import("fastify").FastifyInstance} fastify
 * @param {import("fastify").FastifyPluginOptions} options
 * @return {Promise<void>}
 */
const routes = async (fastify, options) => {
    fastify.get("/decompression_job/:jobId", async (req, resp) => {
        const result = await fastify.dbManager.getDecompressionJob(req.params.jobId);
        resp.send(result);
    });

    fastify.post("/decompression_job", async (req, resp) => {
        const result = await fastify.dbManager.insertDecompressionJob(req.body);
        resp.send(result);
    });

    fastify.get("/stats", async (req, resp) => {
        const result = await fastify.dbManager.getStats();
        console.log(result);
        resp.send(result);
    });
};

export default routes;
