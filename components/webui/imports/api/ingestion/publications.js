import {Mongo} from "meteor/mongo";

export const StatsCollection = new Mongo.Collection(Meteor.settings.public.StatsCollectionName);
