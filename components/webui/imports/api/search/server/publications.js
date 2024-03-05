import {logger} from "/imports/utils/logger";
import {Meteor} from "meteor/meteor";

import {SearchResultsMetadataCollection} from "../collections";
import {
    MONGO_SORT_ORDER,
    SEARCH_MAX_NUM_RESULTS,
    SEARCH_RESULTS_FIELDS,
} from "../constants";
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
 * @returns {Mongo.Cursor} cursor that provides access to the search results
 */
Meteor.publish(Meteor.settings.public.SearchResultsCollectionName, ({
    jobId,
}) => {
    logger.debug(
        `Subscription '${Meteor.settings.public.SearchResultsCollectionName}'`,
        `jobId=${jobId}`
    );

    const collection = searchJobCollectionsManager.getOrCreateCollection(jobId);

    const findOptions = {
        sort: [
            [SEARCH_RESULTS_FIELDS.TIMESTAMP, MONGO_SORT_ORDER.DESCENDING],
            [SEARCH_RESULTS_FIELDS.ID, MONGO_SORT_ORDER.DESCENDING],
        ],
        limit: SEARCH_MAX_NUM_RESULTS,
        disableOplog: true,
        pollingIntervalMs: 250,
    };

    return collection.find({}, findOptions);
});
