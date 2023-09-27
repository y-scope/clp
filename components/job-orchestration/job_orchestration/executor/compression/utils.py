from contextlib import closing

import json
import pika

RESULTS_QUEUE_NAME = 'results'


def append_message_to_task_results_queue(celery_broker_url: str, declare_queue: bool, message: dict):
    with closing(pika.BlockingConnection(pika.URLParameters(celery_broker_url))) as conn:
        with closing(conn.channel()) as channel:
            channel.tx_select()
            if declare_queue:
                channel.queue_declare(RESULTS_QUEUE_NAME)

            channel.basic_publish(exchange='', routing_key=RESULTS_QUEUE_NAME, body=json.dumps(message).encode('utf-8'))
            channel.tx_commit()
