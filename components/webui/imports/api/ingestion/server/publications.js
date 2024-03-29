import {logger} from "/imports/utils/logger";
import {Meteor} from "meteor/meteor";

import {STATS_COLLECTION_ID_COMPRESSION, StatsCollection} from "../collections";
import StatsDbManager from "./StatsDbManager";


/**
 * @type {number}
 */
const STATS_REFRESH_INTERVAL_MS = 5000;

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
 * @returns {Promise<void>}
 */
const refreshCompressionStats = async () => {
    if (Meteor.server.stream_server.all_sockets().length === 0) {
        return;
    }

    const stats = await statsDbManager.getCompressionStats();
    const filter = {
        id: STATS_COLLECTION_ID_COMPRESSION,
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
 * @param {mysql.Connection} sqlDbConnection
 * @param {object} tableNames
 * @param {string} tableNames.clpArchivesTableName
 * @param {string} tableNames.clpFilesTableName
 * @throws {Error} on error.
 */
const initStatsDbManager = (sqlDbConnection, {
    clpArchivesTableName,
    clpFilesTableName,
}) => {
    statsDbManager = new StatsDbManager(sqlDbConnection, {
        clpArchivesTableName,
        clpFilesTableName,
    });

    refreshMeteorInterval = Meteor.setInterval(refreshCompressionStats, STATS_REFRESH_INTERVAL_MS);
};

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
 *
 * @returns {Mongo.Cursor}
 */
Meteor.publish(Meteor.settings.public.StatsCollectionName, async () => {
    logger.debug(`Subscription '${Meteor.settings.public.StatsCollectionName}'`);

    await refreshCompressionStats();

    const filter = {
        id: STATS_COLLECTION_ID_COMPRESSION,
    };

    return StatsCollection.find(filter);
});

export {initStatsDbManager, deinitStatsDbManager};
