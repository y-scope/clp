import {Meteor} from "meteor/meteor";

import {logger} from "/imports/utils/logger";

import {
    STATS_COLLECTION_ID,
    StatsCollection,
} from "../collections";
import StatsDbManager from "./StatsDbManager";


const STATS_REFRESH_INTERVAL_MILLIS = 5000;

/**
 * @type {StatsDbManager|null}
 */
let statsDbManager = null;

/**
 * @type {number|null}
 */
let refreshMeteorInterval = null;

/**
 * Updates the compression statistics in the StatsCollection.
 *
 * @return {Promise<void>}
 */
const refreshCompressionStats = async () => {
    if (0 === Meteor.server.stream_server.all_sockets().length) {
        return;
    }

    const stats = await statsDbManager.getCompressionStats();
    const filter = {
        _id: STATS_COLLECTION_ID.COMPRESSION,
    };
    const modifier = {
        $set: stats,
    };
    const options = {
        upsert: true,
    };

    await StatsCollection.updateAsync(filter, modifier, options);
};

/**
 * Initializes the StatsDbManager and sets the refresh interval for compression stats updates.
 *
 * @param {import("mysql2/promise").Pool} sqlDbConnPool
 * @param {object} tableNames
 * @param {string} tableNames.clpArchivesTableName
 * @param {string} tableNames.clpFilesTableName
 * @throws {Error} on error.
 */
const initStatsDbManager = (sqlDbConnPool, {
    clpArchivesTableName,
    clpFilesTableName,
}) => {
    statsDbManager = new StatsDbManager(sqlDbConnPool, {
        clpArchivesTableName,
        clpFilesTableName,
    });

    refreshMeteorInterval = Meteor.setInterval(
        refreshCompressionStats,
        STATS_REFRESH_INTERVAL_MILLIS
    );
};

/**
 * De-initializes the StatsDbManager by clearing the refreshMeteorInterval.
 */
const deinitStatsDbManager = () => {
    if (null !== refreshMeteorInterval) {
        Meteor.clearInterval(refreshMeteorInterval);
        refreshMeteorInterval = null;
    }
};

/**
 * Updates and publishes compression statistics.
 *
 * @param {string} publicationName
 * @return {Mongo.Cursor}
 */
Meteor.publish(Meteor.settings.public.StatsCollectionName, async () => {
    logger.debug(`Subscription '${Meteor.settings.public.StatsCollectionName}'`);

    await refreshCompressionStats();

    const filter = {
        _id: STATS_COLLECTION_ID.COMPRESSION,
    };

    return StatsCollection.find(filter);
});

export {
    deinitStatsDbManager,
    initStatsDbManager,
};
