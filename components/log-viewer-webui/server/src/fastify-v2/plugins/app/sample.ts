import { FastifyInstance } from 'fastify'
import fp from 'fastify-plugin'

declare module 'fastify' {
  export interface FastifyInstance {
    ExamplePlugin: ReturnType<typeof createExamplePlugin>;
  }
}

/**
 * Example to demonstrate how to create a Fastify plugin.
 *
 * TODO: Remove example when new webui app code is complete.
 */
function createExamplePlugin (fastify: FastifyInstance) {
    return {
      /**
       * Returns `Example`.
       *
       * @returns
       */
      async getExample (): Promise<string> {
        return `Example`;
      },
    }
  }


export default fp(
  function (fastify) {
    fastify.decorate('ExamplePlugin', createExamplePlugin(fastify))
  },
  {
    name: 'ExamplePlugin',
  }
)
