// Reference: https://github.com/fastify/demo/blob/main/src/plugins/external/rate-limit.ts

import fastifyRateLimit from "@fastify/rate-limit";
import {FastifyInstance} from "fastify";


export const autoConfig = (fastify: FastifyInstance) => {
    return {
        max: fastify.config.RATE_LIMIT_MAX,
        timeWindow: "1 minute",
    };
};

export default fastifyRateLimit;
