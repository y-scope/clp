import {Meteor} from "meteor/meteor";

import {logger} from "/imports/utils/logger";
import {
    MONGO_SORT_BY_ID,
    MONGO_SORT_ORDER,
} from "/imports/utils/mongo";

import {SearchResultsMetadataCollection} from "../collections";
import {
    SEARCH_MAX_NUM_RESULTS,
    SEARCH_RESULTS_FIELDS,
} from "../constants";
import {searchJobCollectionsManager} from "./collections";


/**
 * The interval, in milliseconds, at which the Meteor Mongo collection is polled.
 */
const COLLECTION_POLL_INTERVAL_MILLIS = 250;

/**
 * Publishes search results metadata for a specific job.
 *
 * @param {string} publicationName
 * @param {object} props
 * @param {string} props.searchJobId of the search operation
 * @return {Mongo.Cursor} cursor that provides access to the search results metadata.
 */
Meteor.publish(Meteor.settings.public.SearchResultsMetadataCollectionName, ({searchJobId}) => {
    logger.debug(
        `Subscription '${Meteor.settings.public.SearchResultsMetadataCollectionName}'`,
        `searchJobId=${searchJobId}`
    );

    const filter = {
        _id: searchJobId.toString(),
    };

    return SearchResultsMetadataCollection.find(filter);
});

/**
 * Publishes search results for a specific search job.
 *
 * @param {string} publicationName
 * @param {object} props
 * @param {string} props.searchJobId
 * @param {boolean} props.isExpectingUpdates pass true to
 * observe result changes with polling rather than op-log.
 * @return {Mongo.Cursor} cursor that provides access to the search results.
 */
Meteor.publish(Meteor.settings.public.SearchResultsCollectionName, ({
    searchJobId,
    isExpectingUpdates,
}) => {
    logger.debug(
        `Subscription '${Meteor.settings.public.SearchResultsCollectionName}'`,
        `searchJobId=${searchJobId}`
    );

    const collection = searchJobCollectionsManager.getOrCreateCollection(searchJobId);
    const findOptions = {
        sort: [
            /* eslint-disable @stylistic/js/array-element-newline */
            [SEARCH_RESULTS_FIELDS.TIMESTAMP, MONGO_SORT_ORDER.DESCENDING],
            MONGO_SORT_BY_ID,
            /* eslint-enable @stylistic/js/array-element-newline */
        ],
        limit: SEARCH_MAX_NUM_RESULTS,
        // disable oplog should be always true since
        // enable w/o a sort specifier is not supported
        disableOplog: true,
        pollingIntervalMs: (false === isExpectingUpdates) ?
            COLLECTION_POLL_INTERVAL_MILLIS :
            Infinity,
    };

    return collection.find({}, findOptions);
});

/**
 * Publishes search aggregation results for a specific aggregation job.
 *
 * @param {string} publicationName
 * @param {object} props
 * @param {string} props.aggregationJobId
 * @param {boolean} props.isExpectingUpdates pass true to
 * observe result changes with polling rather than op-log.
 * @return {Mongo.Cursor} cursor that provides access to the aggregation results.
 */
Meteor.publish(Meteor.settings.public.AggregationResultsCollectionName, ({
    aggregationJobId,
    isExpectingUpdates,
}) => {
    const collection = searchJobCollectionsManager.getOrCreateCollection(aggregationJobId);
    const findOptions = {
        // disable oplog should be always true since
        // enable w/o a sort specifier is not supported
        disableOplog: true,
        pollingIntervalMs: (false === isExpectingUpdates) ?
            COLLECTION_POLL_INTERVAL_MILLIS :
            Infinity,
    };

    return collection.find({}, findOptions);
});
