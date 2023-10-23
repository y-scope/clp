import {Meteor} from "meteor/meteor";

import {SEARCH_SERVER_STATE_COLLECTION_NAME, SearchResultsMetadataCollection} from "../publications";
import {currentServerStateList} from "./query_handler_mediator";

const MyCollections = {};
const createCollectionsIfNotExist = (sessionId, jobID) => {
    const collectionName = `${Meteor.settings.public.SearchResultsCollectionName}_${jobID}`;

    if ((MyCollections[sessionId] !== undefined) &&
        (MyCollections[sessionId][Meteor.settings.public.SearchResultsCollectionName]._name === collectionName)) {
        return;
    }
    MyCollections[sessionId] = {
        [Meteor.settings.public.SearchResultsCollectionName]: null,
    };
    MyCollections[sessionId][Meteor.settings.public.SearchResultsCollectionName] =
        new Mongo.Collection(collectionName);
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
    return SearchResultsMetadataCollection.find({}, findOptions);
});

Meteor.publish(Meteor.settings.public.SearchResultsCollectionName, function search_results_publication({
                                                                                                           visibleTimeRange,
                                                                                                           fieldToSortBy,
                                                                                                           visibleSearchResultsLimit
                                                                                                       }) {
    if ((!currentServerStateList[this.userId]) || !(currentServerStateList[this.userId].jobID)) {
        return []
    }
    let selector = {};
    if (null !== visibleTimeRange.begin && null !== visibleTimeRange.end) {
        selector["timestamp"] = {
            "$gte": visibleTimeRange.begin, "$lte": visibleTimeRange.end
        };
    }

    let findOptions = {
        limit: visibleSearchResultsLimit, disableOplog: true, pollingIntervalMs: 250
    };
    if (fieldToSortBy) {
        let sort = {};
        sort[fieldToSortBy.name] = fieldToSortBy.direction;
        sort["_id"] = fieldToSortBy.direction;
        findOptions["sort"] = sort;
    }

    createCollectionsIfNotExist(this.userId, currentServerStateList[this.userId].jobID);
    return MyCollections[this.userId][Meteor.settings.public.SearchResultsCollectionName].find(selector, findOptions);
});
