import logging

from celery import Celery, signals
from clp_py_utils.clp_logging import set_json_formatter_on_handlers
from clp_py_utils.telemetry import init_telemetry, shutdown_telemetry

from job_orchestration.executor.compress import celeryconfig

app = Celery("compress")
app.config_from_object(celeryconfig)


@signals.after_setup_logger.connect
@signals.after_setup_task_logger.connect
def setup_json_logging(logger: logging.Logger | None = None, **_: object) -> None:
    """
    Use CLP's JSON formatter for loggers configured by Celery.

    :param logger: Logger configured by Celery, if one was provided by the signal.
    :param _: Additional Celery signal keyword arguments. Unused.
    """
    if logger is not None:
        set_json_formatter_on_handlers(logger)


@signals.worker_process_init.connect
def setup_telemetry(**kwargs) -> None:
    init_telemetry()


@signals.worker_process_shutdown.connect
def teardown_telemetry(**kwargs) -> None:
    shutdown_telemetry()


if "__main__" == __name__:
    app.start()
