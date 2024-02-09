import {Mongo} from "meteor/mongo";
import {INVALID_JOB_ID, SearchSignal} from "./constants";


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
            lastEvent: SearchSignal.NONE,
            errorMsg: null,
            numTotalResults: -1,
        });
    }
};

/**
 * Adds the given sort to the find options for a MongoDB collection.
 * @param {Object|null} fieldToSortBy An object mapping field names to the direction to sort by
 * (ASC = 1, DESC = -1).
 * @param {Object} findOptions
 */
const addSortToMongoFindOptions = (fieldToSortBy, findOptions) => {
    if (fieldToSortBy) {
        findOptions["sort"] = {
            [fieldToSortBy.name]: fieldToSortBy.direction,
            _id: fieldToSortBy.direction,
        };
    }
};

export {addSortToMongoFindOptions, initSearchEventCollection, SearchResultsMetadataCollection};
