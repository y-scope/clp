import {Mongo} from "meteor/mongo";
import {
    INVALID_JOB_ID,
    SEARCH_SIGNAL
} from "./constants";


/**
 * A MongoDB collection for storing metadata about search results.
 */
const SearchResultsMetadataCollection = new Mongo.Collection(
    Meteor.settings.public.SearchResultsMetadataCollectionName);

/**
 * Initializes the search event collection by inserting a default document if the collection is
 * empty.
 */
const initSearchEventCollection = () => {
    // create the collection if not exists
    if (SearchResultsMetadataCollection.countDocuments() === 0) {
        SearchResultsMetadataCollection.insert({
            _id: INVALID_JOB_ID.toString(),
            lastEvent: SEARCH_SIGNAL.NONE,
            errorMsg: null,
            numTotalResults: -1,
        });
    }
};

export {
    initSearchEventCollection,
    SearchResultsMetadataCollection,
};
