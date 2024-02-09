import {logger} from "/imports/utils/logger";
import {Meteor} from "meteor/meteor";

import {addSortToMongoFindOptions, SearchResultsMetadataCollection} from "../collections";
import {searchJobCollectionsManager} from "./collections";


/**
 * Publishes search results metadata for a specific job.
 *
 * @param {string} publicationName
 * @param {string} jobId of the search operation
 *
 * @returns {Mongo.Cursor} cursor that provides access to the search results metadata
 */
Meteor.publish(Meteor.settings.public.SearchResultsMetadataCollectionName, ({jobId}) => {
    logger.debug(`Subscription '${Meteor.settings.public.SearchResultsMetadataCollectionName}'`,
        `jobId=${jobId}`);

    const filter = {
        _id: jobId.toString(),
    };

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
    jobId,
    fieldToSortBy,
    visibleSearchResultsLimit,
}) => {
    logger.debug(`Subscription '${Meteor.settings.public.SearchResultsCollectionName}'`,
        `jobId=${jobId}, fieldToSortBy=${fieldToSortBy}, ` +
        `visibleSearchResultsLimit=${visibleSearchResultsLimit}`);

    const collection = searchJobCollectionsManager.getOrCreateCollection(jobId);

    const findOptions = {
        limit: visibleSearchResultsLimit,
        disableOplog: true,
        pollingIntervalMs: 250,
    };
    addSortToMongoFindOptions(fieldToSortBy, findOptions);

    return collection.find({}, findOptions);
});
