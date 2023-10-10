import {Mongo} from "meteor/mongo";

export const SEARCH_SERVER_STATE_COLLECTION_NAME = "search-server-state";
export const SearchServerStateCollection = new Mongo.Collection(SEARCH_SERVER_STATE_COLLECTION_NAME);
