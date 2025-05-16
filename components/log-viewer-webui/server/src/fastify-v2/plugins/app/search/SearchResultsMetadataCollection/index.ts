import settings from "@settings" with {type: "json"};
import {FastifyInstance} from "fastify";
import fp from "fastify-plugin";

import type {SearchResultsMetadataDocument} from "./typings.js";


/**
 * Creates a MongoDB collection for search results metadata.
 *
 * @param fastify
 * @return MongoDB collection
 * @throws {Error} if the MongoDB database is not found.
 */
const createSearchResultsMetadataCollection = (fastify: FastifyInstance) => {
    if ("undefined" === typeof fastify.mongo.db) {
        throw new Error("MongoDB database not found");
    }

    return fastify.mongo.db.collection<SearchResultsMetadataDocument>(
        settings.MongoDbSearchResultsMetadataCollectionName
    );
};

declare module "fastify" {
    export interface FastifyInstance {
        SearchResultsMetadataCollection: ReturnType<typeof createSearchResultsMetadataCollection>;
    }
}

export default fp(
    (fastify) => {
        fastify.decorate(
            "SearchResultsMetadataCollection",
            createSearchResultsMetadataCollection(fastify)
        );
    },
    {
        name: "SearchResultsMetadataCollection",
    }
);
