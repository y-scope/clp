from celery import Celery, signals

from job_orchestration.executor.query import celeryconfig
from job_orchestration.executor.telemetry import init_telemetry, shutdown_telemetry

app = Celery("query")
app.config_from_object(celeryconfig)

init_telemetry()


@signals.worker_shutdown.connect
def worker_shutdown_handler(**kwargs):
    shutdown_telemetry()


if "__main__" == __name__:
    app.start()
