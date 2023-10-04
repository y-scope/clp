import {Mongo} from "meteor/mongo";

export const SEARCH_SERVER_STATE_COLLECTION_NAME = "search-server-state";
export const SearchServerStateCollection = new Mongo.Collection(SEARCH_SERVER_STATE_COLLECTION_NAME);
export const SearchResultsCollection = new Mongo.Collection(Meteor.settings.public.SearchResultsCollectionName);
export const SearchResultsMetadataCollection = new Mongo.Collection(Meteor.settings.public.SearchResultsMetadataCollectionName);
