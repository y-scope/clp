// Reference: https://github.com/fastify/demo/blob/main/src/app.ts

import path from 'node:path'
import fastifyAutoload from '@fastify/autoload'
import { FastifyInstance, FastifyPluginOptions } from 'fastify'

import appV1 from "../app.js";

export const options = {
  ajv: {
    customOptions: {
      coerceTypes: 'array',
      removeAdditional: 'all'
    }
  }
}

export default async function serviceApp (
  fastify: FastifyInstance,
  opts: FastifyPluginOptions
) {
  delete opts.skipOverride // This option only serves testing purpose
  // This loads all external plugins defined in plugins/external
  // those should be registered first as application plugins might depend on them
  await fastify.register(fastifyAutoload, {
    dir: path.join(import.meta.dirname, 'plugins/external'),
    options: { ...opts }
  })

  /* eslint-disable no-warning-comments */
  // TODO: Refactor to old webui code to use more modular fastify style. For now the
  // old webui code is temporary loaded as a separate plugin.
  await fastify.register(function (fastify) {
    fastify.register(appV1, {
      sqlDbUser: fastify.config.CLP_DB_USER,
      sqlDbPass: fastify.config.CLP_DB_PASS,
    })
  })

  //await fastify.register(appV1, {
  //  sqlDbUser: fastify.config.CLP_DB_USER,
  //  sqlDbPass: fastify.config.CLP_DB_PASS,
  //})

  // Loads all application plugins defined in plugins/app
  fastify.register(fastifyAutoload, {
    dir: path.join(import.meta.dirname, 'plugins/app'),
    options: { ...opts }
  })

  // loads all plugins routes
  fastify.register(fastifyAutoload, {
   dir: path.join(import.meta.dirname, 'routes'),
   autoHooks: true,
   cascadeHooks: true,
   options: { ...opts }
  })

  fastify.setErrorHandler((err, request, reply) => {
    fastify.log.error(
      {
        err,
        request: {
          method: request.method,
          url: request.url,
          query: request.query,
          params: request.params
        }
      },
      'Unhandled error occurred'
    )

    reply.code(err.statusCode ?? 500)

    let message = 'Internal Server Error'
    if (err.statusCode && err.statusCode < 500) {
      message = err.message
    }

    return { message }
  })

  // An attacker could search for valid URLs if your 404 error handling is not rate limited.
  fastify.setNotFoundHandler(
    {
      preHandler: fastify.rateLimit({
        max: 3,
        timeWindow: 500
      })
    },
    (request, reply) => {
      request.log.warn(
        {
          request: {
            method: request.method,
            url: request.url,
            query: request.query,
            params: request.params
          }
        },
        'Resource not found'
      )

      reply.code(404)

      return { message: 'Not Found' }
    })
}