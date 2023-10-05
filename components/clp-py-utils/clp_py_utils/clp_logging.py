import logging

# Setup logging
# Create logger
logger = logging.getLogger("compression-job-handler")
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)
# Prevents double logging from sub loggers for specific jobs
logger.propagate = False

LOGGING_LEVEL_MAPPING = {
    'INFO': logging.INFO,
    'DEBUG': logging.DEBUG,
    'WARN': logging.WARNING,
    'WARNING': logging.WARNING,
    'ERROR': logging.ERROR,
    'CRITICAL': logging.CRITICAL
}


def get_supported_logging_level():
    return [i for i in LOGGING_LEVEL_MAPPING.keys()]


def is_valid_logging_level(level: str):
    return level in LOGGING_LEVEL_MAPPING


def get_logging_level(level: str):
    if not is_valid_logging_level(level):
        logger.warning(f"Invalid logging level: {level}, using INFO as default")
        return logging.INFO
    return LOGGING_LEVEL_MAPPING[level]
