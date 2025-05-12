
import { FastifyInstance } from 'fastify'
import fp from 'fastify-plugin'
import {Nullable} from "../../../typings/common.js";
import {Collection, Document} from "mongodb";

declare module 'fastify' {
  export interface FastifyInstance {
    SearchJobCollectionsManager: ReturnType<typeof createSearchJobCollectionsManager>;
  }
}

/**
 * Class to keep track of MongoDB collections created for search jobs, ensuring all collections
 * have unique names.
 */
function createSearchJobCollectionsManager (fastify: FastifyInstance) {
  const mongo = fastify.mongo.MONGO2
  if ("undefined" === typeof mongo) {
    throw new Error("MongoDB fastify plugin not found")
  }

  const db = mongo.db
  if ("undefined" === typeof db) {
    throw new Error("MongoDB database not found")
  }

  const collections: Map<string, Nullable<Collection<Document>>> = new Map();

  return {
    /**
     * Gets, or if it doesn't exist, creates a MongoDB collection named with the given job ID.
     *
     * @param jobId
     * @return MongoDB collection
     * @throws {Error} if the collection was already dropped.
     */
    async getOrCreateCollection (jobId: number) {
        const name = jobId.toString();
        if ("undefined" === typeof collections.get(name)) {
            collections.set(name, db.collection(name));
        } else if (null === collections.get(name)) {
            throw new Error(
                `Collection ${name} has been dropped.`
            );
        }
        return collections.get(name);
    },


    async dropCollection (jobId: number) {
        const name = jobId.toString();
        const collection = collections.get(name);
        if ("undefined" === typeof collection || null === collection) {
            throw new Error(`Collection ${name} not found`);
        }

        await collection.drop();
        collections.set(name, null);
    },
  }
}

export default fp(
  function (fastify) {
    fastify.decorate('SearchJobCollectionsManager', createSearchJobCollectionsManager(fastify))
  },
  {
    name: 'SearchJobCollectionsManager',
  }
)