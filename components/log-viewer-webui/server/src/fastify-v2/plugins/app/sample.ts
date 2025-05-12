
import { FastifyInstance } from 'fastify'
import fp from 'fastify-plugin'

declare module 'fastify' {
  export interface FastifyInstance {
    sampleApp: ReturnType<typeof createSamplePlugin>;
  }
}

/**
 * Sample code to demonstrate how to create a Fastify plugin.
 *
 * TODO: Remove sample code when new webui app code is ready.
 */
function createSamplePlugin (fastify: FastifyInstance) {
    return {
      /**
       * Returns a sample string.
       *
       * @returns  A sample string.
       */
      async getSample (): Promise<string> {
        return `sample`;
      },
    }
  }


export default fp(
  function (fastify) {
    fastify.decorate('samplePlugin', createSamplePlugin(fastify))
  },
  {
    name: 'samplePlugin',
  }
)