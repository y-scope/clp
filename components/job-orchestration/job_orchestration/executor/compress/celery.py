from celery import Celery

from job_orchestration.executor.compress import celeryconfig

app = Celery("compress")
app.config_from_object(celeryconfig)

if "__main__" == __name__:
    app.start()
