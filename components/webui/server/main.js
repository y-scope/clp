import {initSearchEventCollection} from "/imports/api/search/collections";
import {initLogger} from "/imports/utils/logger";
import {Meteor} from "meteor/meteor";

import "/imports/api/ingestion/collections";
import "/imports/api/ingestion/server/publications";
import "/imports/api/search/server/collections";
import "/imports/api/search/server/methods";
import "/imports/api/search/server/publications";
import "/imports/api/user/server/methods";
import {deinitDbManagers, initDbManagers} from "../imports/utils/DbManager";


const DEFAULT_LOGS_DIR = ".";
const DEFAULT_LOGGING_LEVEL = Meteor.isDevelopment ? "DEBUG" : "INFO";

/**
 * Parses environment variables into config values for the application.
 *
 * @returns {Object} An object containing config values including the SQL database credentials,
 *                   logs directory, and logging level.
 * @throws {Error} if the required environment variables are undefined, it exits the process with an
 *                 error.
 */
const parseEnvVars = () => {
    const CLP_DB_USER = process.env["CLP_DB_USER"];
    const CLP_DB_PASS = process.env["CLP_DB_PASS"];

    if ([CLP_DB_USER, CLP_DB_PASS].includes(undefined)) {
        console.error("Environment variables CLP_DB_USER and CLP_DB_PASS must be defined");
        process.exit(1);
    }

    const WEBUI_LOGS_DIR = process.env["WEBUI_LOGS_DIR"] || DEFAULT_LOGS_DIR;
    const WEBUI_LOGGING_LEVEL = process.env["WEBUI_LOGGING_LEVEL"] || DEFAULT_LOGGING_LEVEL;

    return {
        CLP_DB_USER,
        CLP_DB_PASS,
        WEBUI_LOGS_DIR,
        WEBUI_LOGGING_LEVEL,
    };
};

Meteor.startup(async () => {
    const envVars = parseEnvVars();

    initLogger(envVars.WEBUI_LOGS_DIR, envVars.WEBUI_LOGGING_LEVEL, Meteor.isDevelopment);

    await initDbManagers({
            dbHost: Meteor.settings.private.SqlDbHost,
            dbPort: Meteor.settings.private.SqlDbPort,
            dbName: Meteor.settings.private.SqlDbName,
            dbUser: envVars.CLP_DB_USER,
            dbPassword: envVars.CLP_DB_PASS,
        }, {
            searchJobsTableName: Meteor.settings.private.SqlDbSearchJobsTableName,
            clpArchivesTableName: Meteor.settings.private.SqlDbClpArchivesTableName,
            clpFilesTableName: Meteor.settings.private.SqlDbClpFilesTableName,
        },
    );

    initSearchEventCollection();
});

process.on("exit", async (code) => {
    console.log(`Node.js is about to exit with code: ${code}`);
    await deinitDbManagers();
});
