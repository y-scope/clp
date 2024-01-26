import {Meteor} from "meteor/meteor";

import {SearchResultsMetadataCollection, getCollection, MY_MONGO_DB} from "../collections";

Meteor.publish(Meteor.settings.public.SearchResultsMetadataCollectionName, ({jobId}) => {
    const filter = {
        _id: jobId.toString()
    }

    return SearchResultsMetadataCollection.find(filter);
});

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
