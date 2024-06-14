/**
 * Creates example routes.
 *
 * @param {import("fastify").FastifyInstance} fastify
 * @param {import("fastify").FastifyPluginOptions} options
 * @return {Promise<void>}
 */
const routes = async (fastify, options) => {
    fastify.get("/examples/get/:name", async (req, resp) => {
        return {msg: `Hello, ${req.params.name}!`};
    });

    fastify.post("/examples/post", async (req, resp) => {
        return {msg: `Goodbye, ${req.body.name}!`};
    });
};

export default routes;
