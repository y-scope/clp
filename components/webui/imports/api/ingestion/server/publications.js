import {Meteor} from "meteor/meteor";
import {logger} from "../../../utils/logger";

import {StatsCollection} from "../collections";

import StatsDbManager from "./StatsDbManager";

/**
 * @type {StatsDbManager|null}
 */
let statsDbManager = null;

/**
 * @param {mysql.Connection} sqlDbConnection
 * @param {object} tableNames
 * @param {string} tableNames.clpArchivesTableName
 * @param {string} tableNames.clpFilesTableName
 * @param {string} clpArchivesTableName
 * @param {string} clpFilesTableName
 * @returns {void}
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
};

/**
 * Updates and Publishes compression statistics.
 *
 * @param {string} publicationName
 *
 * @returns {Mongo.Cursor}
 */
Meteor.publish(Meteor.settings.public.StatsCollectionName, async () => {
    // logger.debug(`Subscription '${Meteor.settings.public.StatsCollectionName}'`);

    const stats = await statsDbManager.getCompressionStats();

    const filter = {
        id: "compression_stats",
    };
    const modifier = {
        $set: stats,
    };
    const options = {
        upsert: true,
    };
    await StatsCollection.updateAsync(filter, modifier, options);

    return StatsCollection.find(filter);
});

export {initStatsDbManager};
