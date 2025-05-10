import {Pool} from "mysql2/promise";

import {
    CompressionJobsCollection,
    STATS_COLLECTION_ID,
    StatsCollection,
} from "./collections.js";
import CompressionDbManager from "./CompressionDbManager.js";
import StatsDbManager from "./StatsDbManager.js";


const COMPRESSION_JOBS_REFRESH_INTERVAL_MILLIS = 1000;

const STATS_REFRESH_INTERVAL_MILLIS = 5000;

let compressionDbManager: CompressionDbManager | null = null;

let statsDbManager: StatsDbManager | null = null;

let compressionJobsRefreshTimeout: NodeJS.Timeout | null = null;

let statsRefreshInterval: NodeJS.Timeout | null = null;

let lastUpdateTimestampSeconds: number = 0;

/**
 * Updates the compression statistics in the StatsCollection.
 */
const refreshCompressionStats = async () => {
    const stats = await statsDbManager?.getCompressionStats();
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
 * Updates the compression jobs in the CompressionJobsCollection.
 *
 * @return
 */
const refreshCompressionJobs = async () => {
    if (null !== compressionJobsRefreshTimeout) {
        // Clear the timeout in case this method is not called due to the timeout expiring.
        clearTimeout(compressionJobsRefreshTimeout);
        compressionJobsRefreshTimeout = null;
    }

    const jobs = await compressionDbManager?.getCompressionJobs(
        lastUpdateTimestampSeconds
    );

    if (jobs && 0 !== jobs.length) {
        // `refreshCompressionJobs()` shall not be run concurrently
        // and therefore incurs no race condition.
        // eslint-disable-next-line require-atomic-updates
        lastUpdateTimestampSeconds = jobs[0].retrieval_time;
    }

    const operations = jobs?.map((doc) => ({
        updateOne: {
            filter: {_id: doc._id},
            update: {$set: doc},
            upsert: true,
        },
    }));

    if (0 !== operations.length) {
        await CompressionJobsCollection.rawCollection().bulkWrite(operations);
    }

    // `refreshCompressionJobs()` shall not be run concurrently and therefore incurs no race
    // condition.
    // eslint-disable-next-line require-atomic-updates
    compressionJobsRefreshTimeout = setTimeout(
        refreshCompressionJobs,
        COMPRESSION_JOBS_REFRESH_INTERVAL_MILLIS
    );
};

/**
 * Initializes the CompressionDbManager and starts a timeout timer (`compressionJobsRefreshTimeout`)
 * for compression job updates.
 *
 * @param sqlDbConnPool
 * @param compressionJobsTableName
 * @throws {Error} on error.
 */
const initCompressionDbManager = (sqlDbConnPool: Pool, compressionJobsTableName: string) => {
    compressionDbManager = new CompressionDbManager(sqlDbConnPool, compressionJobsTableName);

    compressionJobsRefreshTimeout = setTimeout(
        refreshCompressionJobs,
        COMPRESSION_JOBS_REFRESH_INTERVAL_MILLIS
    );
};

/**
 * De-initializes the CompressionDbManager by clearing the timeout timer for compression job
 * updates (`refreshCompressionJobs`).
 */
const deinitCompressionDbManager = () => {
    if (null !== compressionJobsRefreshTimeout) {
        clearTimeout(compressionJobsRefreshTimeout);
        compressionJobsRefreshTimeout = null;
    }
};

/**
 * Initializes the StatsDbManager and starts an interval timer (`refreshMeteorInterval`) for
 * compression stats updates.
 *
 * @param sqlDbConnPool
 * @param tableNames
 * @param tableNames.clpArchivesTableName
 * @param tableNames.clpFilesTableName
 * @throws {Error} on error.
 */
const initStatsDbManager = (
    sqlDbConnPool: Pool,
    {clpArchivesTableName, clpFilesTableName}: {
        clpArchivesTableName: string;
        clpFilesTableName: string;
    }
) => {
    statsDbManager = new StatsDbManager(sqlDbConnPool, {
        clpArchivesTableName,
        clpFilesTableName,
    });

    statsRefreshInterval = setInterval(
        refreshCompressionStats,
        STATS_REFRESH_INTERVAL_MILLIS
    );
};

/**
 * De-initializes the StatsDbManager by clearing the interval timer for compression stats updates
 * (`refreshMeteorInterval`).
 */
const deinitStatsDbManager = () => {
    if (null !== statsRefreshInterval) {
        clearInterval(statsRefreshInterval);
        statsRefreshInterval = null;
    }
};

export {
    deinitCompressionDbManager,
    deinitStatsDbManager,
    initCompressionDbManager,
    initStatsDbManager,
};
