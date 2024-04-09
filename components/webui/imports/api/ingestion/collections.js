import {Mongo} from "meteor/mongo";


/**
 * Enum representing the statistics collection IDs.
 *
 * @enum {string}
 */
const STATS_COLLECTION_ID = Object.freeze({
    COMPRESSION: "compression",
});

const StatsCollection = new Mongo.Collection(Meteor.settings.public.StatsCollectionName);

export {
    STATS_COLLECTION_ID,
    StatsCollection,
};
