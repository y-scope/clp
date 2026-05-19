from celery import Celery

from job_orchestration.executor.compress import celeryconfig
from job_orchestration.executor.telemetry import init_telemetry

app = Celery("compress")
app.config_from_object(celeryconfig)

init_telemetry()

if "__main__" == __name__:
    app.start()
