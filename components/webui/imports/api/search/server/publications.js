import {Meteor} from "meteor/meteor";

import {SEARCH_SERVER_STATE_COLLECTION_NAME} from "../publications";
import {currentServerStateList} from "./query_handler_mediator";

const MyCollections = {};
const createCollectionsIfNotExist = (sessionId) => {
    if (!(sessionId in MyCollections)) {
        MyCollections[sessionId] = {
            [Meteor.settings.public.SearchResultsCollectionName]: null,
            [Meteor.settings.public.SearchResultsMetadataCollectionName]: null,
        };
        MyCollections[sessionId][Meteor.settings.public.SearchResultsCollectionName] =
            new Mongo.Collection(`${Meteor.settings.public.SearchResultsCollectionName}_${sessionId}`);
        MyCollections[sessionId][Meteor.settings.public.SearchResultsMetadataCollectionName] =
            new Mongo.Collection(`${Meteor.settings.public.SearchResultsMetadataCollectionName}_${sessionId}`);
    }
}

// TODO: revisit: there is no need to isolate this collection per user/session because it is already done so by meteor?
Meteor.publish(SEARCH_SERVER_STATE_COLLECTION_NAME, function () {
    this.added(SEARCH_SERVER_STATE_COLLECTION_NAME, "main", currentServerStateList[this.userId]);
    this.ready();

    const interval = Meteor.setInterval(() => {
        this.changed(SEARCH_SERVER_STATE_COLLECTION_NAME, "main", currentServerStateList[this.userId]);
        this.ready();
    }, 100);

    this.onStop(() => {
        Meteor.clearInterval(interval);
    });
});

Meteor.publish(Meteor.settings.public.SearchResultsMetadataCollectionName, function () {
    let findOptions = {
        disableOplog: true, pollingIntervalMs: 250
    };
    createCollectionsIfNotExist(this.userId);
    return MyCollections[this.userId][Meteor.settings.public.SearchResultsMetadataCollectionName].find({}, findOptions);
});

Meteor.publish(Meteor.settings.public.SearchResultsCollectionName, function search_results_publication({
                                                                                                           visibleTimeRange,
                                                                                                           fieldToSort,
                                                                                                           visibleSearchResultsLimit
                                                                                                       }) {
    let selector = {};
    if (null !== visibleTimeRange.begin && null !== visibleTimeRange.end) {
        selector["timestamp"] = {
            "$gte": visibleTimeRange.begin, "$lte": visibleTimeRange.end
        };
    }

    let findOptions = {
        limit: visibleSearchResultsLimit, disableOplog: true, pollingIntervalMs: 250
    };
    if (fieldToSort) {
        let sort = {};
        sort[fieldToSort.name] = fieldToSort.direction;
        sort["_id"] = fieldToSort.direction;
        findOptions["sort"] = sort;
    }

    createCollectionsIfNotExist(this.userId);
    return MyCollections[this.userId][Meteor.settings.public.SearchResultsCollectionName].find(selector, findOptions);
});
