import {
    FastifyPluginAsyncTypebox,
    Type
  } from '@fastify/type-provider-typebox'

/**
 * Sample code to demonstrate how to create a Fastify route.
 *
 * TODO: Remove sample code when new webui app code is ready.
 */
  const plugin: FastifyPluginAsyncTypebox = async (fastify) => {
    const { sampleApp } = fastify
    fastify.get(
      '/sample',
      {
        schema: {
          response: {
            200: Type.Object({
              message: Type.String()
            })
          }
        }
      },
      async function () {
        const sampleMessage = await sampleApp.getSample();
        return { message: sampleMessage };
      }
    )
  }

  export default plugin