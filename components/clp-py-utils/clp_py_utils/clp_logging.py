"""Shared logging helpers for CLP Python components."""

import logging
import os
import pathlib
from typing import get_args, Literal

import structlog
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
# ``ProcessorFormatter``.
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

# Processor chain for stdlib ``LogRecord`` objects that did not originate from structlog.
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

structlog.configure(
    processors=[
        *_STRUCTLOG_PROCESSORS,
        structlog.stdlib.ProcessorFormatter.wrap_for_formatter,
    ],
    logger_factory=structlog.stdlib.LoggerFactory(),
    cache_logger_on_first_use=True,
)


def get_logging_formatter() -> logging.Formatter:
    """Return a JSON formatter for both structlog-originated and stdlib log records."""
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
    """Set CLP's JSON log formatter on all handlers currently attached to *logger*."""
    formatter = get_logging_formatter()
    for handler in logger.handlers:
        handler.setFormatter(formatter)


def get_logger(name: str) -> logging.Logger:
    """Return a logger configured with CLP's console formatter."""
    logger = logging.getLogger(name)
    # Setup console logging
    logging_console_handler = logging.StreamHandler()
    logging_console_handler.setFormatter(get_logging_formatter())
    logger.addHandler(logging_console_handler)
    # Prevent double logging from sub loggers
    logger.propagate = False
    return logger


def set_logging_level(logger: logging.Logger, level: str | None) -> None:
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
