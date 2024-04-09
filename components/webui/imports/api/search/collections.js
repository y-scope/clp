import {Mongo} from "meteor/mongo";


/**
 * A MongoDB collection for storing metadata about search results.
 */
const SearchResultsMetadataCollection = new Mongo.Collection(
    Meteor.settings.public.SearchResultsMetadataCollectionName
);

export {SearchResultsMetadataCollection};
