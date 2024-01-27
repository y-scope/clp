import {Meteor} from "meteor/meteor";

import "/imports/api/search/server/methods";
import "/imports/api/search/server/publications";
import "/imports/api/user/server/methods";

import {initSQL, deinitSQL} from "../imports/api/search/sql";
import {initSearchEventCollection} from "../imports/api/search/collections";
import {initLogger} from "../imports/utils/logger";

const DEFAULT_LOGS_DIR = ".";
const DEFAULT_LOGGING_LEVEL = Meteor.isDevelopment ? "DEBUG" : "INFO";

function parseArgs() {
    const CLP_DB_HOST = process.env["CLP_DB_HOST"];
    const CLP_DB_PORT = process.env["CLP_DB_PORT"];
    const CLP_DB_NAME = process.env["CLP_DB_NAME"];
    const CLP_DB_USER = process.env["CLP_DB_USER"];
    const CLP_DB_PASS = process.env["CLP_DB_PASS"];

    if ([CLP_DB_HOST, CLP_DB_PORT, CLP_DB_NAME, CLP_DB_USER, CLP_DB_PASS].includes(undefined)) {
        console.error(
            "Environment variables CLP_DB_URL, CLP_DB_USER and CLP_DB_PASS need to be defined");
        process.exit(1);
    }

    const CLP_LOGS_DIR = process.env["CLP_LOGS_DIR"] || DEFAULT_LOGS_DIR;
    const CLP_LOGGING_LEVEL = process.env["CLP_LOGGING_LEVEL"] || DEFAULT_LOGGING_LEVEL;

    return {
        CLP_DB_HOST,
        CLP_DB_PORT,
        CLP_DB_NAME,
        CLP_DB_USER,
        CLP_DB_PASS,
        CLP_LOGS_DIR,
        CLP_LOGGING_LEVEL,
    };
}

Meteor.startup(async () => {
    const args = parseArgs();

    initLogger(args.CLP_LOGS_DIR, args.CLP_LOGGING_LEVEL, Meteor.isDevelopment);

    await initSQL(
        args.CLP_DB_HOST,
        parseInt(args.CLP_DB_PORT),
        args.CLP_DB_NAME,
        args.CLP_DB_USER,
        args.CLP_DB_PASS,
    );

    initSearchEventCollection();
});

process.on("exit", async (code) => {
    console.log(`Node.js is about to exit with code: ${code}`);
    await deinitSQL();
});
