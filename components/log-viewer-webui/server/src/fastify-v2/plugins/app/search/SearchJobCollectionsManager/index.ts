import {FastifyInstance} from "fastify";
import fp from "fastify-plugin";
import type {
    Collection,
    Db,
} from "mongodb";

import {Nullable} from "../../../../../typings/common.js";
import {
    CollectionDroppedError,
    SearchResultsDocument,
} from "./typings.js";


/**
 * Class to keep track of MongoDB collections created for search jobs, ensuring all collections
 * have unique names.
 */
class SearchJobCollectionsManager {
    #jobIdToCollectionsMap: Map<string, Nullable<Collection<SearchResultsDocument>>> =
        new Map();

    #db: Db;

    private constructor (db: Db) {
        this.#db = db;
    }

    /**
     * Creates a new SearchJobCollectionsManager.
     *
     * @param fastify
     * @return
     * @throws {Error} if the MongoDB database is not found.
     */
    static create (fastify: FastifyInstance): SearchJobCollectionsManager {
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
     * @throws {CollectionDroppedError} if the collection was already dropped.
     */
    async getOrCreateCollection (jobId: number): Promise<Collection<SearchResultsDocument>> {
        const name = jobId.toString();
        const collection = this.#jobIdToCollectionsMap.get(name);
        if ("undefined" === typeof collection) {
            this.#jobIdToCollectionsMap.set(name, this.#db.collection(name));
        } else if (null === collection) {
            throw new CollectionDroppedError(name);
        }


        // `collections.get(name)` will always return a valid collection since:
        // -  Function creates a new collection if it doesn't exist in map.
        // -  Throws an error if the collection was already dropped.
        // Therefore, we can safely cast to `Collection<SearchResultsDocument>`.
        return this.#jobIdToCollectionsMap.get(name) as Collection<SearchResultsDocument>;
    }

    /**
     * Drops the MongoDB collection associated with the given job ID.
     *
     * @param jobId
     * @throws {Error} if the collection does not exist or has already been dropped.
     */
    async dropCollection (jobId: number) {
        const name = jobId.toString();
        const collection = this.#jobIdToCollectionsMap.get(name);
        if ("undefined" === typeof collection) {
            throw new Error(`Collection ${name} not found`);
        } else if (null === collection) {
            throw new CollectionDroppedError(name);
        }

        await collection.drop();
        this.#jobIdToCollectionsMap.set(name, null);
    }
}

declare module "fastify" {
    export interface FastifyInstance {
        SearchJobCollectionsManager: SearchJobCollectionsManager;
    }
}

export default fp(
    (fastify) => {
        fastify.decorate(
            "SearchJobCollectionsManager",
            SearchJobCollectionsManager.create(fastify)
        );
    },
    {
        name: "SearchJobCollectionsManager",
    }
);
