import logging

from celery import Celery, signals
from clp_py_utils.clp_logging import set_json_formatter_on_handlers

from job_orchestration.executor.compress import celeryconfig

app = Celery("compress")
app.config_from_object(celeryconfig)


@signals.after_setup_logger.connect
@signals.after_setup_task_logger.connect
def setup_json_logging(logger: logging.Logger | None = None, **_: object) -> None:
    """Use CLP's JSON formatter for loggers configured by Celery."""
    if logger is not None:
        set_json_formatter_on_handlers(logger)


if "__main__" == __name__:
    app.start()
