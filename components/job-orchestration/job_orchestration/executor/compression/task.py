import json
import os
from contextlib import closing

import pika
from celery.utils.log import get_task_logger

from job_orchestration.executor.celery import app
from . import fs_to_fs_compress_method

logger = get_task_logger(__name__)

from clp_py_utils.clp_io_config import ClpIoConfig, PathsToCompress


@app.task()
def compress(job_id: int, task_id: int, clp_io_config_json: str, paths_to_compress_json: str,
             database_connection_params):
    clp_home = os.getenv('CLP_HOME')
    data_dir = os.getenv('CLP_DATA_DIR')
    logs_dir = os.getenv('CLP_LOGS_DIR')
    celery_broker_url = os.getenv('BROKER_URL')

    logger.debug(f'CLP_HOME: {clp_home}')
    logger.info(f'COMPRESSING job_id={job_id} task_id={task_id}')

    clp_io_config = ClpIoConfig.parse_raw(clp_io_config_json)
    paths_to_compress = PathsToCompress.parse_raw(paths_to_compress_json)

    message = {'job_id': job_id, 'task_id': task_id, 'status': 'COMPRESSING'}

    with closing(pika.BlockingConnection(pika.URLParameters(celery_broker_url))) as conn:
        with closing(conn.channel()) as channel:
            channel.tx_select()
            channel.queue_declare('results')

            channel.basic_publish(exchange='', routing_key='results',
                                  body=json.dumps(message).encode('utf-8'))
            channel.tx_commit()
            logger.info(f'COMPRESSION STARTED job_id={job_id} task_id={task_id}')

    if 'fs' == clp_io_config.input.type and 'fs' == clp_io_config.output.type:
        compression_successful, worker_output = \
            fs_to_fs_compress_method.compress(
                clp_io_config, clp_home, data_dir, logs_dir, str(job_id), str(task_id),
                paths_to_compress, database_connection_params)
    else:
        raise NotImplementedError

    if compression_successful:
        message['status'] = 'COMPLETED'
        message['total_uncompressed_size'] = worker_output['total_uncompressed_size']
        message['total_compressed_size'] = worker_output['total_compressed_size']
    else:
        message['status'] = 'FAILED'
        message['error_message'] = worker_output['error_message']

    with closing(pika.BlockingConnection(pika.URLParameters(celery_broker_url))) as conn:
        with closing(conn.channel()) as channel:
            channel.tx_select()
            channel.basic_publish(exchange='', routing_key='results', body=json.dumps(message).encode('utf-8'))
            channel.tx_commit()
            logger.info(f'COMPRESSION COMPLETED job_id={job_id} task_id={task_id}')
