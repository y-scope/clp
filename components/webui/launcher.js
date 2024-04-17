/**
 * Production launcher for CLP WebUI, which redirects Meteor server stderr to rotated error logs
 * files in a specified directory for error monitoring.
 *
 * To avoid duplicated installations of dependencies, use the same `node_modules` for the server
 * by setting envvar NODE_PATH="./programs/server/npm/node_modules", assuming this script is
 * placed under the same directory where the bundled `main.js` is located.
 *
 * This is not intended for development use. For development, please refer to README.md in the
 * component root for launching a development server with Meteor-specific error messages print
 * to the console.
 *
 * ENVIRONMENT VARIABLES:
 *   - NODE_PATH: path to node_modules including "winston" and "winston-daily-rotate-file"
 *   - WEBUI_LOGS_DIR: path to error logs directory
 * SCRIPT USAGE:
 *   - usage: node /path/to/launcher.js /path/to/main.js
 */

const {spawn} = require("child_process");
const winston = require("winston");
require("winston-daily-rotate-file");


const DEFAULT_LOGS_DIR = ".";

const MAX_LOGS_FILE_SIZE = "100m";
const MAX_LOGS_RETENTION_DAYS = "30d";

/**
 * Creates a logger using winston module.
 *
 * @param {string} logsDir directory where the log files will be saved.
 * @return {object} the logger object
 */
const getLogger = (logsDir) => {
    return winston.createLogger({
        format: winston.format.combine(
            winston.format.timestamp(),
            winston.format.printf((info) => {
                return JSON.stringify({
                    timestamp: info.timestamp,
                    level: info.level,
                    label: info.label,
                    message: info.message,
                });
            }),
        ),
        transports: [
            new winston.transports.DailyRotateFile({
                datePattern: "YYYY-MM-DD-HH",
                dirname: logsDir,
                filename: "webui_error-%DATE%.log",
                maxFiles: MAX_LOGS_RETENTION_DAYS,
                maxSize: MAX_LOGS_FILE_SIZE,
            }),
        ],
    });
};


/**
 * Runs a script with logging support.
 *
 * @param {string} logsDir path where the logs will be stored
 * @param {string} scriptPath path of the script to be executed
 */
const runScript = (logsDir, scriptPath) => {
    const logger = getLogger(logsDir);
    const script = spawn(process.argv0, [scriptPath]);

    script.stderr.on("data", (data) => {
        logger.error(data.toString());
    });

    script.on("close", (code) => {
        console.log(`Child process exited with code ${code}`);
    });
};

/**
 * Parses the command line arguments and retrieves the values for the
 * WEBUI_LOGS_DIR and scriptPath variables.
 *
 * @return {object} containing the values for WEBUI_LOGS_DIR and scriptPath
 */
const parseArgs = () => {
    const WEBUI_LOGS_DIR = process.env.WEBUI_LOGS_DIR || DEFAULT_LOGS_DIR;
    // eslint-disable-next-line prefer-destructuring
    const scriptPath = process.argv[2];

    return {
        WEBUI_LOGS_DIR,
        scriptPath,
    };
};

/**
 * The main function of the program.
 *
 * This function is the entry point of the program.
 *
 * @return {void}
 */
const main = () => {
    const args = parseArgs();

    runScript(args.WEBUI_LOGS_DIR, args.scriptPath);
};

main();
