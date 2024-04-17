import {Mongo} from "meteor/mongo";


/**
 * @typedef {object} SearchResultsMetadata
 * @property {string} _id
 * @property {string|null} errorMsg
 * @property {SearchSignal} lastSignal
 * @property {number} numTotalResults
 */

/**
 * A MongoDB collection for storing metadata about search results.
 */
const SearchResultsMetadataCollection = new Mongo.Collection(
    Meteor.settings.public.SearchResultsMetadataCollectionName
);

export {SearchResultsMetadataCollection};
