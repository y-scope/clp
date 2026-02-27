import logging
import os
import pathlib
from typing import get_args, Literal

LoggingLevel = Literal[
    "INFO",
    "DEBUG",
    "WARN",
    "WARNING",
    "ERROR",
    "CRITICAL",
]


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


def set_logging_level(logger: logging.Logger, level: str | None):
    if level is None:
        logger.setLevel(logging.INFO)
        return
    if level not in get_args(LoggingLevel):
        logger.warning(f"Invalid logging level: {level}, using INFO as default")
        logger.setLevel(logging.INFO)
        return

    logger.setLevel(level)


def configure_logging(
    logger: logging.Logger,
    component_name: str,
):
    """
    Configures file logging and the logging level for a logger using environment variables.

    If ``CLP_LOGS_DIR`` is set, a :class:`logging.FileHandler` writing to
    ``<CLP_LOGS_DIR>/<component_name>.log`` is added to the logger.
    ``CLP_LOGGING_LEVEL`` is used to set the logging level (defaults to INFO if unset/invalid).

    :param logger: The logger to configure.
    :param component_name: Used as the log filename stem and in log messages.
    """
    logs_dir = os.getenv("CLP_LOGS_DIR")
    if logs_dir:
        log_file = pathlib.Path(logs_dir) / f"{component_name}.log"
        logging_file_handler = logging.FileHandler(filename=log_file, encoding="utf-8")
        logging_file_handler.setFormatter(get_logging_formatter())
        logger.addHandler(logging_file_handler)
    set_logging_level(logger, os.getenv("CLP_LOGGING_LEVEL"))
