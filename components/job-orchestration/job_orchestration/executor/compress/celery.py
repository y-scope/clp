import logging

from celery import Celery, signals

from job_orchestration.executor.compress import celeryconfig
from job_orchestration.executor.utils import add_container_log_handler

app = Celery("compress")
app.config_from_object(celeryconfig)


@signals.after_setup_logger.connect
def _on_after_setup_logger(logger: logging.Logger, **kwargs) -> None:
    add_container_log_handler(logger)


if "__main__" == __name__:
    app.start()
