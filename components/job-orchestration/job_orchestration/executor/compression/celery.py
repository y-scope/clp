from celery import Celery
from . import celeryconfig  # type: ignore

app = Celery("compression")
app.config_from_object(celeryconfig)
