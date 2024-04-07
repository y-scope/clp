import {Meteor} from "meteor/meteor";

import {logger} from "/imports/utils/logger";
import {MONGO_SORT_BY_ID} from "/imports/utils/mongo";

import {
    CompressionJobsCollection,
    STATS_COLLECTION_ID_COMPRESSION,
    StatsCollection,
} from "../collections";
import {
    COMPRESSION_JOB_WAITING_STATES,
    COMPRESSION_JOBS_TABLE_COLUMN_NAMES,
    COMPRESSION_MAX_RETRIEVE_JOBS,
} from "../constants";
import CompressionDbManager from "./CompressionDbManager";
import StatsDbManager from "./StatsDbManager";


/**
 * The refresh interval (in milliseconds) for updating compression statistics.
 */
const STATS_REFRESH_INTERVAL_MS = 5000;

/**
 * The refresh interval (in milliseconds) for updating compression jobs.
 */
const COMPRESSION_JOBS_REFRESH_INTERVAL_MS = 1000;

/**
 * @type {StatsDbManager|null}
 */
let statsDbManager = null;

/**
 * @type {CompressionDbManager|null}
 */
let compressionDbManager = null;

/**
 * @type {number|null}
 */
let statsRefreshInterval = null;

/**
 * @type {number|null}
 */
let compressionJobsRefreshTimeout = null;

/**
 * Updates the compression statistics in the StatsCollection.
 *
 * @returns {Promise<void>}
 */
const refreshCompressionStats = async () => {
    if (0 === Meteor.server.stream_server.all_sockets().length) {
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

    statsRefreshInterval = Meteor.setInterval(refreshCompressionStats, STATS_REFRESH_INTERVAL_MS);
};

const deinitStatsDbManager = () => {
    if (null !== statsRefreshInterval) {
        Meteor.clearInterval(statsRefreshInterval);
        statsRefreshInterval = null;
    }
};

/**
 * Updates the compression jobs in the CompressionJobsCollection.
 *
 * @returns {Promise<void>}
 */
const refreshCompressionJobs = async () => {
    if (null !== compressionJobsRefreshTimeout) {
        Meteor.clearTimeout(compressionJobsRefreshTimeout);
        compressionJobsRefreshTimeout = null;
    }
    if (0 === Meteor.server.stream_server.all_sockets().length) {
        compressionJobsRefreshTimeout = Meteor.setTimeout(
            refreshCompressionJobs,
            COMPRESSION_JOBS_REFRESH_INTERVAL_MS
        );

        return;
    }

    const pendingJobIdList = await CompressionJobsCollection.find({
        [COMPRESSION_JOBS_TABLE_COLUMN_NAMES.STATUS]: {
            $in: COMPRESSION_JOB_WAITING_STATES,
        },
    })
        .fetch()
        .map((job) => (
            // eslint-disable-next-line no-underscore-dangle
            job._id
        ));

    const jobs = await compressionDbManager.getCompressionJobs(
        COMPRESSION_MAX_RETRIEVE_JOBS,
        pendingJobIdList
    );

    const operations = jobs.map((doc) => ({
        updateOne: {
            // eslint-disable-next-line no-underscore-dangle
            filter: {_id: doc._id},
            update: {$set: doc},
            upsert: true,
        },
    }));

    if (0 !== operations.length) {
        await CompressionJobsCollection.rawCollection().bulkWrite(operations);
    }
    compressionJobsRefreshTimeout = Meteor.setTimeout(
        refreshCompressionJobs,
        COMPRESSION_JOBS_REFRESH_INTERVAL_MS
    );
};

/**
 * @param {import("mysql2/promise").Pool} sqlDbConnPool
 * @param {object} tableNames
 * @param {string} tableNames.compressionJobsTableName
 * @throws {Error} on error.
 */
const initCompressionDbManager = (sqlDbConnPool, {
    compressionJobsTableName,
}) => {
    compressionDbManager = new CompressionDbManager(sqlDbConnPool, {
        compressionJobsTableName,
    });

    compressionJobsRefreshTimeout = Meteor.setTimeout(
        refreshCompressionJobs,
        COMPRESSION_JOBS_REFRESH_INTERVAL_MS
    );
};

const deinitCompressionDbManager = () => {
    if (null !== compressionJobsRefreshTimeout) {
        Meteor.clearTimeout(compressionJobsRefreshTimeout);
        compressionJobsRefreshTimeout = null;
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

/**
 * Updates and publishes compression job statuses.
 *
 * @param {string} publicationName
 *
 * @returns {Mongo.Cursor}
 */
Meteor.publish(Meteor.settings.public.CompressionJobsCollectionName, async () => {
    logger.debug(`Subscription '${Meteor.settings.public.CompressionJobsCollectionName}'`);

    await refreshCompressionJobs();

    const findOptions = {
        sort: [MONGO_SORT_BY_ID],
    };

    return CompressionJobsCollection.find({}, findOptions);
});

export {
    deinitCompressionDbManager,
    deinitStatsDbManager,
    initCompressionDbManager,
    initStatsDbManager,
};
