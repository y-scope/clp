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
 * The maximum value (2^31 - 1) that can be used as a polling interval in JavaScript.
 * Reference: https://developer.mozilla.org/en-US/docs/Web/API/setTimeout#maximum_delay_value
 */
// eslint-disable-next-line no-magic-numbers
const JS_MAX_DELAY_VALUE = (2 ** 31) - 1;

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
 * @param {boolean} props.isExpectingUpdates Whether the subscriber
 * expects that the collection will be updated.
 * @return {Mongo.Cursor} cursor that provides access to the search results.
 */
Meteor.publish(Meteor.settings.public.SearchResultsCollectionName, ({
    searchJobId,
    isExpectingUpdates,
}) => {
    logger.debug(
        `Subscription '${Meteor.settings.public.SearchResultsCollectionName}'`,
        `searchJobId=${searchJobId}`,
        `isExpectingUpdates=${isExpectingUpdates}`
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
        disableOplog: true,
        pollingIntervalMs: isExpectingUpdates ?
            COLLECTION_POLL_INTERVAL_MILLIS :
            JS_MAX_DELAY_VALUE,
    };

    return collection.find({}, findOptions);
});

/**
 * Publishes search aggregation results for a specific aggregation job.
 *
 * @param {string} publicationName
 * @param {object} props
 * @param {string} props.aggregationJobId
 * @param {boolean} props.isExpectingUpdates Whether the subscriber
 * expects that the collection will be updated.
 * @return {Mongo.Cursor} cursor that provides access to the aggregation results.
 */
Meteor.publish(Meteor.settings.public.AggregationResultsCollectionName, ({
    aggregationJobId,
    isExpectingUpdates,
}) => {
    logger.debug(
        `Subscription '${Meteor.settings.public.AggregationResultsCollectionName}'`,
        `aggregationJobId=${aggregationJobId}`,
        `isExpectingUpdates=${isExpectingUpdates}`
    );

    const collection = searchJobCollectionsManager.getOrCreateCollection(aggregationJobId);
    const findOptions = {
        disableOplog: true,
        pollingIntervalMs: isExpectingUpdates ?
            COLLECTION_POLL_INTERVAL_MILLIS :
            JS_MAX_DELAY_VALUE,
    };

    return collection.find({}, findOptions);
});
