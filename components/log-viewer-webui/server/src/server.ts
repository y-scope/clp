// https://github.com/fastify/demo/blob/main/src/server.ts

import Fastify from 'fastify'
import fp from 'fastify-plugin'

import closeWithGrace from 'close-with-grace'

import serviceApp from './fastify-v2/app.js'

/**
 * Do not use NODE_ENV to determine what logger (or any env related feature) to use
 * @see {@link https://www.youtube.com/watch?v=HMM7GJC5E2o}
 */
function getLoggerOptions () {
  // Only if the program is running in an interactive terminal.
  if (process.stdout.isTTY) {
    return {
      level: 'info',
      transport: {
        target: 'pino-pretty',
        options: {
          translateTime: 'HH:MM:ss Z',
          ignore: 'pid,hostname'
        }
      }
    }
  }

  return { level: process.env.LOG_LEVEL ?? 'silent' }
}

const app = Fastify({
  logger: getLoggerOptions(),
  ajv: {
    customOptions: {
      // Change type of data to match type keyword.
      coerceTypes: 'array',
      // Remove additional body properties.
      removeAdditional: 'all'
    }
  }
})

async function init () {
  // fp must be used to override default error handler.
  app.register(fp(serviceApp))

  closeWithGrace(
    { delay: Number(process.env.FASTIFY_CLOSE_GRACE_DELAY || 500) },
    async ({ err }) => {
      if (err != null) {
        app.log.error(err)
      }

      await app.close()
    }
  )

  await app.ready()

  try {
    await app.listen({ port:  Number(process.env.PORT ?? 3000) })
  } catch (err) {
    app.log.error(err)
    process.exit(1)
  }
}

init()