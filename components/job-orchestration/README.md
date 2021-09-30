# CLP Job Orchestration

This Python module contains CLP's scheduler and worker to handle distributed compression. 
CLP's Compression Job Handler can be used to interface and submit compression jobs to the CLP scheduler.

🔔 clp-job-orchestration is part of a larger CLP package that can be built from 
[clp-packager](https://github.com/y-scope/clp-packager).

## Installation

```bash
pip3 install -r requirements.txt --target <clp-package>/lib/python3/site-packages
cp -R clp_py_utils <clp-package>/lib/python3/site-packages
```

## Usage

### Running the `scheduler`

```bash
PYTHONPATH=<clp_home/lib/python3/site-packages> \
  BROKER_URL=amqp://<rabbitmq_user>:<rabbitmq_password>@<rabbitmq_host>:<rabbitmq_port> \
  python3 -m job_orchestration.scheduler.scheduler --config <clp config file path>
```

### Running the `executor`

```bash
PYTHONPATH=<clp_home/lib/python3/site-packages> \
  CLP_HOME=<clp_home> \
  CLP_DATA_DIR=<clp data directory> \
  CLP_LOGS_DIR=<clp log directory> \
  BROKER_URL=amqp://<rabbitmq_user>:<rabbitmq_password>@<rabbitmq_host>:<rabbitmq_port> \
  RESULT_BACKEND=rpc://<rabbitmq_user>:<rabbitmq_password>@<rabbitmq_host>:<rabbitmq_port> \
  celery -A executor worker --loglevel INFO -Q compression
```
