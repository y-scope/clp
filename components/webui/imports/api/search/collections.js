import {Mongo} from "meteor/mongo";
import {INVALID_JOB_ID, SearchSignal} from "./constants";

/**
 * A MongoDB collection for storing metadata about search results.
 *
 * @constant
 */
export const SearchResultsMetadataCollection = new Mongo.Collection(Meteor.settings.public.SearchResultsMetadataCollectionName);
/**
 * Initializes the search event collection by inserting a default document if the collection is empty.
 */
export const initSearchEventCollection = () => {
    // create the collection if not exists
    if (SearchResultsMetadataCollection.find().count() === 0) {
        SearchResultsMetadataCollection.insert({
            _id: INVALID_JOB_ID.toString(),
            lastEvent: SearchSignal.NONE,
            errorMsg: null,
            numTotalResults: -1
        });
    }
}

/**
 * Object to store references to MongoDB collections.
 * This should only be accessed by the server; clients should use a React referenced-object.
 *
 * @constant
 * @type {Object}
 */export const MY_MONGO_DB = {}

/**
 * Retrieves a Mongo Collection object by name, creating it if it does not already exist.
 * This is to adhere to Meteor.js's restriction against creating more than one Collection object
 * with the same name.
 *
 * @param {Object} dbRef database object where collections are stored
 * @param {string} name of the collection to retrieve or create
 * @returns {Mongo.Collection}
 */
export const getCollection = (dbRef, name) => {
    if (dbRef[name] === undefined) {
        dbRef[name] = new Mongo.Collection(name);
    }
    return dbRef[name];
};
