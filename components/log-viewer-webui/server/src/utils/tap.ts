// Reference: https://github.com/fastify/demo/blob/main/test/helper.ts

import { FastifyInstance,} from 'fastify'
import { build as buildApplication } from 'fastify-cli/helper.js'
import path from 'node:path'

import { Test } from 'tap';

const AppPath = path.join(import.meta.dirname, '../app.ts')

export function config () {
  return {
    skipOverride: true, // Register our application with fastify-plugin
  }
}

// automatically build and tear down our instance
export async function build (t: Test) {
  // you can set all the options supported by the fastify CLI command
  const argv = [AppPath]

  // fastify-plugin ensures that all decorators
  // are exposed for testing purposes, this is
  // different from the production setup
  console.log("Building application...");
  const app = (await buildApplication(
    argv,
    config(),
  )) as FastifyInstance
  console.log("Application built.");

    t.teardown(() => {
        console.log("Closing application");
        app.close();
    });

  return app
}