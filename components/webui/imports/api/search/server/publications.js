import {Meteor} from "meteor/meteor";

import {SearchResultsMetadataCollection, getCollection, MY_MONGO_DB} from "../collections";

/**
 * Publishes search results metadata for a specific job.
 *
 * @param {string} publicationName
 * @param {string} jobId of the search operation
 *
 * @returns {Mongo.Cursor} cursor that provides access to the search results metadata
 */
Meteor.publish(Meteor.settings.public.SearchResultsMetadataCollectionName, ({jobId}) => {
    const filter = {
        _id: jobId.toString()
    }

    return SearchResultsMetadataCollection.find(filter);
});

/**
 * Publishes search results for a specific job with optional sorting and result limit.
 *
 * @param {string} publicationName
 * @param {string} jobId of the search operation
 * @param {Object} [fieldToSortBy] used for sorting results
 * @param {number} visibleSearchResultsLimit limit of visible search results
 *
 * @returns {Mongo.Cursor} cursor that provides access to the search results
 */
Meteor.publish(Meteor.settings.public.SearchResultsCollectionName, ({
                                                                        jobId, fieldToSortBy, visibleSearchResultsLimit
                                                                    }) => {
    const collection = getCollection(MY_MONGO_DB, jobId.toString())

    const findOptions = {
        limit: visibleSearchResultsLimit,
        disableOplog: true,
        pollingIntervalMs: 250
    };
    if (fieldToSortBy) {
        const sort = {};
        sort[fieldToSortBy.name] = fieldToSortBy.direction;
        sort["_id"] = fieldToSortBy.direction;
        findOptions["sort"] = sort;
    }

    return collection.find({}, findOptions);
});
