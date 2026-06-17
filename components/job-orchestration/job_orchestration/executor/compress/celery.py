import logging

from celery import Celery, signals
from clp_py_utils.clp_logging import set_formatter_on_handlers

from job_orchestration.executor.compress import celeryconfig

app = Celery("compress")
app.config_from_object(celeryconfig)


@signals.after_setup_logger.connect
@signals.after_setup_task_logger.connect
def configure_logger_formatters(logger: logging.Logger | None = None, **_kwargs: object) -> None:
    if logger is not None:
        set_formatter_on_handlers(logger)


if "__main__" == __name__:
    app.start()
