"""Shared logging helpers for CLP Python components."""

import logging
import os
import pathlib
from collections.abc import Iterator
from contextlib import contextmanager
from contextvars import ContextVar
from typing import get_args, Literal

import structlog
from structlog.stdlib import BoundLogger
from structlog.typing import Processor

LoggingLevel = Literal[
    "INFO",
    "DEBUG",
    "WARN",
    "WARNING",
    "ERROR",
    "CRITICAL",
]

# Processor chain for events emitted through structlog loggers before they reach
# `ProcessorFormatter`.
_STRUCTLOG_PROCESSORS: tuple[Processor, ...] = (
    structlog.stdlib.filter_by_level,
    structlog.contextvars.merge_contextvars,
    structlog.stdlib.add_logger_name,
    structlog.stdlib.add_log_level,
    structlog.stdlib.PositionalArgumentsFormatter(),
    structlog.processors.TimeStamper(fmt="iso", utc=True, key="timestamp"),
    structlog.processors.StackInfoRenderer(),
    structlog.processors.format_exc_info,
    structlog.processors.UnicodeDecoder(),
    structlog.processors.CallsiteParameterAdder(
        {
            structlog.processors.CallsiteParameter.FILENAME,
            structlog.processors.CallsiteParameter.FUNC_NAME,
            structlog.processors.CallsiteParameter.LINENO,
        }
    ),
)

# Processor chain for stdlib `LogRecord` objects that did not originate from structlog.
_FOREIGN_PRE_CHAIN: tuple[Processor, ...] = (
    structlog.contextvars.merge_contextvars,
    structlog.stdlib.add_logger_name,
    structlog.stdlib.add_log_level,
    structlog.stdlib.ExtraAdder(),
    structlog.stdlib.PositionalArgumentsFormatter(),
    structlog.processors.TimeStamper(fmt="iso", utc=True, key="timestamp"),
    structlog.processors.StackInfoRenderer(),
    structlog.processors.format_exc_info,
    structlog.processors.UnicodeDecoder(),
    structlog.processors.CallsiteParameterAdder(
        {
            structlog.processors.CallsiteParameter.FILENAME,
            structlog.processors.CallsiteParameter.FUNC_NAME,
            structlog.processors.CallsiteParameter.LINENO,
        }
    ),
)


def configure_structlog() -> None:
    """Configure structlog with CLP's default processor chain if it is not already configured."""
    if not structlog.is_configured():
        structlog.configure(
            processors=[
                *_STRUCTLOG_PROCESSORS,
                structlog.stdlib.ProcessorFormatter.wrap_for_formatter,
            ],
            logger_factory=structlog.stdlib.LoggerFactory(),
            wrapper_class=BoundLogger,
            cache_logger_on_first_use=True,
        )


def get_structlog_logger(name: str) -> BoundLogger:
    """
    Configure CLP's structured logging defaults and return a structlog logger for a component.

    CLP uses structlog's stdlib integration, so this first configures the stdlib logger's
    handlers with structlog's `ProcessorFormatter`. Use the returned structlog logger for log calls.

    :param name: Name of the logger to create or retrieve.
    :return: A structlog logger configured to emit through CLP's JSON formatter.
    """
    configure_structlog()
    stdlib_logger = get_logger(name)
    configure_logging(stdlib_logger, name)
    return structlog.stdlib.get_logger(name)


def get_logging_formatter() -> logging.Formatter:
    """:return: A JSON log formatter configured with CLP's structlog processors."""
    return structlog.stdlib.ProcessorFormatter(
        # foreign_pre_chain is run for stdlib LogRecord objects that do not go through
        # structlog's processor chain.
        foreign_pre_chain=_FOREIGN_PRE_CHAIN,
        processors=[
            structlog.stdlib.ProcessorFormatter.remove_processors_meta,
            structlog.processors.JSONRenderer(),
        ],
    )


def set_json_formatter_on_handlers(logger: logging.Logger) -> None:
    """
    Set CLP's JSON log formatter on all handlers currently attached to the logger.

    :param logger: Logger whose handlers should use CLP's JSON formatter.
    """
    formatter = get_logging_formatter()
    for handler in logger.handlers:
        handler.setFormatter(formatter)


def get_logger(name: str) -> logging.Logger:
    """
    :param name: Name of the logger to create or retrieve.
    :return: A logger configured with CLP's JSON console formatter.
    """
    logger = logging.getLogger(name)
    # Setup console logging
    logging_console_handler = logging.StreamHandler()
    logging_console_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_console_handler)
    # Prevent double logging from sub loggers
    logger.propagate = False
    return logger


def set_logging_level(logger: logging.Logger, level: str | None) -> None:
    """
    Set a logger to the requested logging level, defaulting to INFO when the level is unset or
    invalid.

    :param logger: Logger whose level should be updated.
    :param level: Requested logging level, or `None` to use INFO.
    """
    if level is None:
        logger.setLevel(logging.INFO)
        return
    if level not in get_args(LoggingLevel):
        logger.warning("Invalid logging level: %s, using INFO as default", level)
        logger.setLevel(logging.INFO)
        return

    logger.setLevel(level)


def configure_logging(
    logger: logging.Logger,
    component_name: str,
) -> None:
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
