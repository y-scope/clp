import { FastifyInstance } from 'fastify';
import fp from 'fastify-plugin';
import { Nullable } from "../../../../typings/common.js";
import type { Db, Collection } from "mongodb";

interface SearchResultsMetadataDocument {
  _id: string;
  orig_file_id: string;
  orig_file_path: string;
  log_event_ix: number;
  timestamp: number;
  message: string;
}

/**
 * Class to keep track of MongoDB collections created for search jobs, ensuring all collections
 * have unique names.
 */
class SearchJobCollectionsManager {
  #collections: Map<string, Nullable<Collection<SearchResultsMetadataDocument>>> = new Map();
  #db: Db;

  private constructor (db: Db) {
      this.#db = db;
  }

  /**
   * Creates a new SearchJobCollectionsManager.
   *
   * @param fastify
   * @returns
   */
  static create(fastify: FastifyInstance): SearchJobCollectionsManager {
    if ("undefined" === typeof fastify.mongo.db) {
      throw new Error("MongoDB database not found");
    }

    return new SearchJobCollectionsManager(fastify.mongo.db);
  }

  /**
   * Gets, or if it doesn't exist, creates a MongoDB collection named with the given job ID.
   *
   * @param jobId
   * @return MongoDB collection
   * @throws {Error} if the collection was already dropped.
   */
  async getOrCreateCollection(jobId: number) {
    const name = jobId.toString();
    if (false === this.#collections.has(name)) {
      this.#collections.set(name, this.#db.collection(name));
    } else if (this.#collections.get(name) === null) {
      throw new Error(`Collection ${name} has been dropped.`);
    }
    return this.#collections.get(name);
  }

  /**
   * Drops the MongoDB collection associated with the given job ID.
   *
   * @param jobId
   * @throws {Error} if the collection does not exist or has already been dropped.
   */
  async dropCollection(jobId: number) {
    const name = jobId.toString();
    const collection = this.#collections.get(name);
    if ("undefined" === typeof collection || null === collection) {
      throw new Error(`Collection ${name} not found`);
    }

    await collection.drop();
    this.#collections.set(name, null);
  }
}

declare module 'fastify' {
  export interface FastifyInstance {
    SearchJobCollectionsManager: SearchJobCollectionsManager;
  }
}

export default fp(
  function (fastify) {
    fastify.decorate('SearchJobCollectionsManager', SearchJobCollectionsManager.create(fastify));
  },
  {
    name: 'SearchJobCollectionsManager',
  }
);