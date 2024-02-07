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
    if (SearchResultsMetadataCollection.countDocuments() === 0) {
        SearchResultsMetadataCollection.insert({
            _id: INVALID_JOB_ID.toString(),
            lastEvent: SearchSignal.NONE,
            errorMsg: null,
            numTotalResults: -1
        });
    }
}
