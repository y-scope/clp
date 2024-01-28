import winston from "winston";
import "winston-daily-rotate-file";
import JSON5 from "json5";

const MAX_LOGS_FILE_SIZE = "100m";
const MAX_LOGS_RETENTION_DAYS = "30d";

let isTraceEnabled = false;

// attribute names should match clp_py_utils.clp_logging.LOGGING_LEVEL_MAPPING
const webuiLoggingLevelToWinstonMap = {
    DEBUG: "debug",
    INFO: "info",
    WARN: "warn",
    WARNING: "warn",
    ERROR: "error",
    CRITICAL: "error",
};

const getStackInfo = () => {
    let info = null;

    const stackList = (new Error()).stack.split("\n");
    const stackInfo = stackList[4];
    const stackRegex = /at\s+(.*)\s+\((.*):(\d+):(\d+)\)/i;
    const stackMatch = stackRegex.exec(stackInfo);

    if (null !== stackMatch && stackMatch.length === 5) {
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


let winstonLogger = null;
const fileLineFuncLog = (level, ...args) => {
    let logMessage = `${args.map(a => ("string" === typeof a) ? a : JSON5.stringify(a)).join(" ")}`;
    let logLabel = "";

    if (true === isTraceEnabled) {
        const stackInfo = getStackInfo();

        if (null !== stackInfo) {
            logMessage = `[${stackInfo.filePath}:${stackInfo.line}] ` + logMessage;
            logLabel = stackInfo.method;
        }
    }

    winstonLogger.log({
        level,
        message: logMessage,
        label: logLabel,
    });
};

export let logger = Object.freeze({
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

export const initLogger = (logsDir, webuiLoggingLevel, _isTraceEnabled = false) => {
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
                filename: "webui-%DATE%.log",
                dirname: logsDir,
                datePattern: "YYYY-MM-DD-HH",
                maxSize: MAX_LOGS_FILE_SIZE,
                maxFiles: MAX_LOGS_RETENTION_DAYS,
            }),
        ],
    });

    logger.info("logger has been initialized");
};