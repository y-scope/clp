import {Meteor} from "meteor/meteor";

import {SEARCH_SERVER_STATE_COLLECTION_NAME, SearchResultsCollection, SearchResultsMetadataCollection} from "../publications";
import {currentServerState} from "./query_handler_mediator";

Meteor.publish(SEARCH_SERVER_STATE_COLLECTION_NAME, function () {
    this.added(SEARCH_SERVER_STATE_COLLECTION_NAME, "main", currentServerState);
    this.ready();

    const interval = Meteor.setInterval(() => {
        this.changed(SEARCH_SERVER_STATE_COLLECTION_NAME, "main", currentServerState);
        this.ready();
    }, 100);

    this.onStop(() => {
        Meteor.clearInterval(interval);
    });
});

Meteor.publish(Meteor.settings.public.SearchResultsMetadataCollectionName, function () {
    let findOptions = {
        disableOplog: true,
        pollingIntervalMs: 250
    };
    return SearchResultsMetadataCollection.find({}, findOptions);
});

Meteor.publish(Meteor.settings.public.SearchResultsCollectionName, function search_results_publication({
                                                                                                           visibleTimeRange,
                                                                                                           fieldToSort,
                                                                                                           visibleSearchResultsLimit
                                                                                                       }) {
    let selector = {};
    if (null !== visibleTimeRange.begin && null !== visibleTimeRange.end) {
        selector["timestamp"] = {
            "$gte": visibleTimeRange.begin,
            "$lte": visibleTimeRange.end
        };
    }

    let findOptions = {
        limit: visibleSearchResultsLimit,
        disableOplog: true,
        pollingIntervalMs: 250
    };
    if (fieldToSort) {
        let sort = {};
        sort[fieldToSort.name] = fieldToSort.direction;
        sort["_id"] = fieldToSort.direction;
        findOptions["sort"] = sort;
    }

    return SearchResultsCollection.find(selector, findOptions);
});
