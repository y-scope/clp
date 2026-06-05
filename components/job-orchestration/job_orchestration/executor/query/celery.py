from celery import Celery
from celery.signals import worker_process_init, worker_process_shutdown
from clp_py_utils.telemetry import init_telemetry, shutdown_telemetry

from job_orchestration.executor.query import celeryconfig

app = Celery("query")
app.config_from_object(celeryconfig)


@worker_process_init.connect
def setup_telemetry(**kwargs) -> None:
    init_telemetry()


@worker_process_shutdown.connect
def teardown_telemetry(**kwargs) -> None:
    shutdown_telemetry()


if "__main__" == __name__:
    app.start()
