import JSON5 from "json5";
import winston from "winston";

import "winston-daily-rotate-file";


const MAX_LOGS_FILE_SIZE = "100m";
const MAX_LOGS_RETENTION_DAYS = "30d";

let winstonLogger = null;
let isTraceEnabled = false;

/* eslint-disable sort-keys */
// attribute names should match clp_py_utils.clp_logging.LOGGING_LEVEL_MAPPING
const webuiLoggingLevelToWinstonMap = {
    DEBUG: "debug",
    INFO: "info",
    WARN: "warn",
    WARNING: "warn",
    ERROR: "error",
    CRITICAL: "error",
};
/* eslint-enable sort-keys */

/* eslint-disable prefer-destructuring, no-magic-numbers */
/**
 * Retrieves information about the calling function's stack trace.
 *
 * @return {object | null} an object containing method, filePath, and line information,
 * or null if the information couldn't be extracted
 */
const getStackInfo = () => {
    let info;

    const stackList = (new Error()).stack.split("\n");
    const stackInfo = stackList[4];
    const stackRegex = /at\s+(.*)\s+\((.*):(\d+):(\d+)\)/i;
    const stackMatch = stackRegex.exec(stackInfo);

    if (null !== stackMatch && 5 === stackMatch.length) {
        info = {
            method: stackMatch[1],
            filePath: stackMatch[2],
            line: stackMatch[3],
        };
    } else {
        const stackRegex2 = /at\s+(.*):(\d*):(\d*)/i;
        const stackMatch2 = stackRegex2.exec(stackInfo);
        info = {
            method: "",
            filePath: stackMatch2[1],
            line: stackMatch2[2],
        };
    }

    return info;
};
/* eslint-enable prefer-destructuring, no-magic-numbers */

/**
 * Logs a message with the specified log level, including optional trace information.
 *
 * @param {string} level of the log message
 * @param {...any} args message or data to be logged
 */
const fileLineFuncLog = (level, ...args) => {
    let logMessage = `${args.map((a) => (("string" === typeof a) ?
        a :
        JSON5.stringify(a))).join(" ")}`;
    let logLabel = "";

    if (true === isTraceEnabled) {
        const stackInfo = getStackInfo();

        if (null !== stackInfo) {
            logMessage = `[${stackInfo.filePath}:${stackInfo.line}] ${logMessage}`;
            logLabel = stackInfo.method;
        }
    }

    winstonLogger.log({
        level: level,
        message: logMessage,
        label: logLabel,
    });
};

/* eslint-disable sort-keys */
/**
 * Logger interface object that provides logging with different levels.
 */
const logger = Object.freeze({
    error: (...args) => (fileLineFuncLog("error", ...args)),
    warn: (...args) => (fileLineFuncLog("warn", ...args)),
    help: (...args) => (fileLineFuncLog("help", ...args)),
    data: (...args) => (fileLineFuncLog("data", ...args)),
    info: (...args) => (fileLineFuncLog("info", ...args)),
    debug: (...args) => (fileLineFuncLog("debug", ...args)),
    prompt: (...args) => (fileLineFuncLog("prompt", ...args)),
    verbose: (...args) => (fileLineFuncLog("verbose", ...args)),
    input: (...args) => (fileLineFuncLog("input", ...args)),
    silly: (...args) => (fileLineFuncLog("silly", ...args)),
});
/* eslint-enable sort-keys */

/**
 * Initializes winston logger with the specified configuration.
 *
 * @param {string} logsDir where log files will be stored.
 * @param {string} webuiLoggingLevel messages higher than this level will be logged
 * @param {boolean} [_isTraceEnabled] whether to log function & file names and line numbers
 */
const initLogger = (logsDir, webuiLoggingLevel, _isTraceEnabled = false) => {
    isTraceEnabled = _isTraceEnabled;

    winstonLogger = winston.createLogger({
        level: webuiLoggingLevelToWinstonMap[webuiLoggingLevel],
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
            new winston.transports.Console(),
            new winston.transports.DailyRotateFile({
                datePattern: "YYYY-MM-DD-HH",
                dirname: logsDir,
                filename: "webui-%DATE%.log",
                maxFiles: MAX_LOGS_RETENTION_DAYS,
                maxSize: MAX_LOGS_FILE_SIZE,
            }),
        ],
    });

    logger.info("logger has been initialized");
};

export {
    initLogger,
    logger,
};
