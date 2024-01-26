import {Mongo} from "meteor/mongo";
import {INVALID_JOB_ID, SearchSignal} from "./constants";


export const SearchResultsMetadataCollection = new Mongo.Collection(Meteor.settings.public.SearchResultsMetadataCollectionName);
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

// this should only be accessed by the server; client should use a React referenced-object
export const MY_MONGO_DB = {}

// Retrieves Mongo Collection object by name if it has been created
// as Meteor.js does not permit creating more than one Collection object with a same name.
export const getCollection = (dbRef, name) => {
    if (dbRef[name] === undefined) {
        dbRef[name] = new Mongo.Collection(name);
    }
    return dbRef[name];
};
