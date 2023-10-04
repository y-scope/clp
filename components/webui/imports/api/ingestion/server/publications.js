import {Meteor} from "meteor/meteor";

import {StatsCollection} from "../publications";

Meteor.publish(Meteor.settings.public.StatsCollectionName, function () {
    return StatsCollection.find({});
});
