import {encode} from "@msgpack/msgpack";

import { ResultSetHeader, RowDataPacket } from 'mysql2';

import {
    QUERY_JOB_STATUS,
    QUERY_JOB_STATUS_WAITING_STATES,
    QUERY_JOB_TYPE,
} from "./constants.js";

import { FastifyInstance } from 'fastify'
import fp from 'fastify-plugin'

import settings from "../../../../settings.json" with {type: "json"};

import { sleep } from "../../../utils/misc.js";

import { Collection, Document } from "mongodb";

/**
 * Interval in milliseconds for polling the completion status of a job.
 */
const JOB_COMPLETION_STATUS_POLL_INTERVAL_MILLIS = 0.5;

/**
 * Enum of the `query_jobs` table's column names.
 *
 * @enum {string}
 */
const QUERY_JOBS_TABLE_COLUMN_NAMES = Object.freeze({
    ID: "id",
    STATUS: "status",
    TYPE: "type",
    JOB_CONFIG: "job_config",
});


declare module 'fastify' {
  export interface FastifyInstance {
    SearchResultsMetadataCollection: ReturnType<typeof createSearchResultsMetadataCollection>;
  }
}
interface SearchResultsMetadata {
    _id: string;
    lastSignal: string;
    errorMsg: string | null;
  }


/**
 * Class for submitting and monitoring query jobs in the database.
 */
function createSearchResultsMetadataCollection (fastify: FastifyInstance) {
    const mongo = fastify.mongo.MONGO2

    if ("undefined" === typeof mongo) {
        throw new Error("MongoDB fastify plugin not found")
    }

    const db = mongo.db
    if ("undefined" === typeof db) {
        throw new Error("MongoDB database not found")
    }

    /**
     * A MongoDB collection for storing metadata about search results.
     */
    const SearchResultsMetadataCollection: Collection<SearchResultsMetadata> = db.collection("test");

  return  SearchResultsMetadataCollection;
}

export default fp(
  function (fastify) {
    fastify.decorate('SearchResultsMetadataCollection', createSearchResultsMetadataCollection(fastify))
  },
  {
    name: 'QueryJobsDbManager',
    dependencies: ['SearchJobCollectionsManager']
  }
)