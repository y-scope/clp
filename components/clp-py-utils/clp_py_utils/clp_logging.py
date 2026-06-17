import logging
import os
import pathlib
from collections.abc import Iterator
from contextlib import contextmanager
from contextvars import ContextVar
from typing import get_args, Literal

LoggingLevel = Literal[
    "INFO",
    "DEBUG",
    "WARN",
    "WARNING",
    "ERROR",
    "CRITICAL",
]


_LOG_CONTEXT: ContextVar[dict[str, object] | None] = ContextVar("clp_log_context", default=None)

_LOG_CONTEXT_FIELD_ORDER = (
    "job_id",
    "query_job_type",
    "task_id",
    "archive_id",
    "dataset",
    "partition_id",
    "celery_task_id",
    "clp_subprocess_pid",
    "query",
    "query_hash",
)


@contextmanager
def bind_log_context(**fields: object) -> Iterator[None]:
    """
    Temporarily adds CLP correlation fields to log records emitted in the current context.

    Fields with ``None`` values are ignored. Nested contexts merge with outer contexts and restore
    the previous context on exit.
    """
    current_context = _LOG_CONTEXT.get() or {}
    new_context = current_context.copy()
    new_context.update({key: value for key, value in fields.items() if value is not None})
    token = _LOG_CONTEXT.set(new_context)
    try:
        yield
    finally:
        _LOG_CONTEXT.reset(token)


def get_log_context() -> dict[str, object]:
    """Returns a copy of the active CLP log correlation context."""
    return (_LOG_CONTEXT.get() or {}).copy()


class ClpContextFormatter(logging.Formatter):
    """Appends active CLP correlation fields to log records."""

    def format(self, record: logging.LogRecord) -> str:
        message = super().format(record)
        context = get_log_context()
        if not context:
            return message

        fields = [f"{key}={context[key]}" for key in _LOG_CONTEXT_FIELD_ORDER if key in context]
        fields.extend(
            f"{key}={value}"
            for key, value in sorted(context.items())
            if key not in _LOG_CONTEXT_FIELD_ORDER
        )
        if len(fields) > 0:
            message = f"{message} [{' '.join(fields)}]"
        return message


def get_logging_formatter() -> logging.Formatter:
    return ClpContextFormatter("%(asctime)s %(name)s [%(levelname)s] %(message)s")


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
