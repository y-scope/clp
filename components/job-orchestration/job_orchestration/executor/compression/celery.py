from celery import Celery

from . import celeryconfig

app = Celery('clp_compression')
app.config_from_object(celeryconfig)

if '__main__' == __name__:
    app.start()
