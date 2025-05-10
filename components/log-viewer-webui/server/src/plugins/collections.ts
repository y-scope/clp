import {Mongo} from "meteor/mongo";

import {
    COMPRESSION_JOBS_COLLECTION_NAME,
    STATS_COLLECTION_NAME,
} from "./constants.js";


const CompressionJobsCollection = new Mongo.Collection(
    COMPRESSION_JOBS_COLLECTION_NAME
);

/**
 * Enum representing the statistics collection IDs.
 *
 * @enum {string}
 */
const STATS_COLLECTION_ID = Object.freeze({
    COMPRESSION: "compression",
});

const StatsCollection = new Mongo.Collection(STATS_COLLECTION_NAME);

export {
    CompressionJobsCollection,
    STATS_COLLECTION_ID,
    StatsCollection,
};
