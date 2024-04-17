import logging

LOGGING_LEVEL_MAPPING = {
    "INFO": logging.INFO,
    "DEBUG": logging.DEBUG,
    "WARN": logging.WARNING,
    "WARNING": logging.WARNING,
    "ERROR": logging.ERROR,
    "CRITICAL": logging.CRITICAL,
}


def get_logging_formatter():
    return logging.Formatter("%(asctime)s %(name)s [%(levelname)s] %(message)s")


def get_logger(name: str):
    logger = logging.getLogger(name)
    # Setup console logging
    logging_console_handler = logging.StreamHandler()
    logging_console_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_console_handler)
    # Prevent double logging from sub loggers
    logger.propagate = False
    return logger


def get_valid_logging_level():
    return [i for i in LOGGING_LEVEL_MAPPING.keys()]


def is_valid_logging_level(level: str):
    return level in LOGGING_LEVEL_MAPPING


def set_logging_level(logger: logging.Logger, level: str):
    if not is_valid_logging_level(level):
        logger.warning(f"Invalid logging level: {level}, using INFO as default")
        logger.setLevel(logging.INFO)
        return
    logger.setLevel(LOGGING_LEVEL_MAPPING[level])
