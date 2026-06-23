import logging
import os
import pathlib
from collections.abc import Iterator
from contextlib import contextmanager
from typing import get_args, Literal

import structlog
from structlog.contextvars import (
    bind_contextvars,
    get_contextvars,
    merge_contextvars,
    reset_contextvars,
)

LoggingLevel = Literal[
    "INFO",
    "DEBUG",
    "WARN",
    "WARNING",
    "ERROR",
    "CRITICAL",
]

_TIMESTAMP_PROCESSOR = structlog.processors.TimeStamper(fmt="iso", utc=True, key="timestamp")
_JSON_RENDERER = structlog.processors.JSONRenderer()


@contextmanager
def bind_log_context(**fields: object) -> Iterator[None]:
    """
    Adds CLP correlation fields to log records emitted in the current context.

    Fields with ``None`` values are ignored. Nested contexts merge with outer contexts and restore
    the previous context on exit.
    """
    tokens = bind_contextvars(**{key: value for key, value in fields.items() if value is not None})
    try:
        yield
    finally:
        reset_contextvars(**tokens)


def get_log_context() -> dict[str, object]:
    """Returns a copy of the active CLP log correlation context."""
    return get_contextvars()


class _ClpJsonFormatter(logging.Formatter):
    """Formats log records as JSON and includes active CLP correlation fields."""

    def format(self, record: logging.LogRecord) -> str:
        method_name = record.levelname.lower()
        event_dict: dict[str, object] = {
            "event": record.getMessage(),
            "logger": record.name,
            "level": method_name,
        }
        if record.exc_info:
            event_dict["exception"] = self.formatException(record.exc_info)
        if record.stack_info:
            event_dict["stack"] = self.formatStack(record.stack_info)

        merge_contextvars(None, method_name, event_dict)
        _TIMESTAMP_PROCESSOR(None, method_name, event_dict)
        rendered_log = _JSON_RENDERER(None, method_name, event_dict)
        if isinstance(rendered_log, bytes):
            return rendered_log.decode("utf-8")
        return rendered_log


def get_logging_formatter() -> logging.Formatter:
    return _ClpJsonFormatter()


def set_formatter_on_handlers(logger: logging.Logger) -> None:
    """Sets CLP's logging formatter on all existing handlers for a logger."""
    formatter = get_logging_formatter()
    for handler in logger.handlers:
        handler.setFormatter(formatter)


def get_logger(name: str) -> logging.Logger:
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
        logger.warning(f"Invalid logging level: {level}, using INFO as default")
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
