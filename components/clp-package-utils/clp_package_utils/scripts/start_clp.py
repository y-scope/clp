import argparse
import json
import logging
import multiprocessing
import os
import pathlib
import socket
import subprocess
import sys
import time
import uuid

import yaml
from pydantic import BaseModel

from clp_package_utils.general import (
    check_dependencies,
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CLPDockerMounts,
    CONTAINER_CLP_HOME,
    container_exists,
    DockerMount,
    DockerMountType,
    generate_container_config,
    get_clp_home,
    is_container_running,
    validate_and_load_compression_queue_credentials_file,
    validate_and_load_config_file,
    validate_and_load_db_credentials_file,
    validate_db_config,
    validate_queue_config,
    validate_results_cache_config,
    validate_webui_config,
    validate_webui_query_handler_config,
    validate_worker_config,
)
from clp_py_utils.clp_config import (
    CLPConfig,
    DB_COMPONENT_NAME,
    COMPRESSION_QUEUE_COMPONENT_NAME,
    Queue,
    RESULTS_CACHE_COMPONENT_NAME,
    SEARCH_QUEUE_COMPONENT_NAME,
    SEARCH_SCHEDULER_COMPONENT_NAME,
    SEARCH_WORKER_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    COMPRESSION_JOB_HANDLER_COMPONENT_NAME,
    WEBUI_COMPONENT_NAME,
    WEBUI_QUERY_HANDLER_COMPONENT_NAME,
)
from job_orchestration.scheduler.constants import QueueName

# Setup logging
# Create logger
logger = logging.getLogger('clp')
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def append_docker_port_settings_for_host_ips(hostname: str, host_port: int, container_port: int, cmd: [str]):
    # Note: We use a set because gethostbyname_ex can return the same IP twice for one hostname
    for ip in set(socket.gethostbyname_ex(hostname)[2]):
        cmd.append('-p')
        cmd.append(f'{ip}:{host_port}:{container_port}')


def wait_for_database_to_init(container_name: str, clp_config: CLPConfig, timeout: int):
    # Try to connect to the database
    begin_time = time.time()
    container_exec_cmd = [
        'docker', 'exec',
        '-it',
        container_name
    ]
    mysqladmin_cmd = [
        'mysqladmin', 'ping',
        '--silent',
        '-h', '127.0.0.1',
        '-u', str(clp_config.database.username),
        f'--password={clp_config.database.password}'
    ]
    cmd = container_exec_cmd + mysqladmin_cmd
    while True:
        try:
            subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)
            return True
        except subprocess.CalledProcessError:
            if time.time() - begin_time > timeout:
                break
            time.sleep(1)

    logger.error(f"Timeout while waiting for {DB_COMPONENT_NAME} to initialize.")
    return False


def start_db(instance_id: str, clp_config: CLPConfig, conf_dir: pathlib.Path):
    component_name = DB_COMPONENT_NAME
    logger.info(f"Starting {component_name}...")

    container_name = f'clp-{component_name}-{instance_id}'
    if is_container_running(container_name):
        logger.info(f"{container_name} already running.")
        return
    elif container_exists(container_name):
        logger.info(f"{container_name} stopped but not removed.")
        return

    db_data_dir = clp_config.data_directory / DB_COMPONENT_NAME
    db_logs_dir = clp_config.logs_directory / DB_COMPONENT_NAME

    validate_db_config(clp_config, db_data_dir, db_logs_dir)

    # Create directories
    db_data_dir.mkdir(exist_ok=True, parents=True)
    db_logs_dir.mkdir(exist_ok=True, parents=True)

    is_root = os.getuid() == 0
    if is_root:
        # Make the directories world-writable so that the default user in the
        # container can write to them
        # TODO Find a less brute-force solution
        db_data_dir.chmod(0o777)
        db_logs_dir.chmod(0o777)

    # Start container
    mounts = [
        DockerMount(DockerMountType.BIND, conf_dir / 'mysql' / 'conf.d', pathlib.Path('/') / 'etc' / 'mysql' / 'conf.d',
                    True),
        DockerMount(DockerMountType.BIND, db_data_dir, pathlib.Path('/') / 'var' / 'lib' / 'mysql'),
        DockerMount(DockerMountType.BIND, db_logs_dir, pathlib.Path('/') / 'var' / 'log' / 'mysql'),
    ]
    cmd = [
        'docker', 'run',
        '-d',
        '--name', container_name,
        '-e', f'MYSQL_ROOT_PASSWORD={clp_config.database.password}',
        '-e', f'MYSQL_USER={clp_config.database.username}',
        '-e', f'MYSQL_PASSWORD={clp_config.database.password}',
        '-e', f'MYSQL_DATABASE={clp_config.database.name}',
    ]
    for mount in mounts:
        cmd.append('--mount')
        cmd.append(str(mount))
    append_docker_port_settings_for_host_ips(clp_config.database.host, clp_config.database.port, 3306, cmd)
    if not is_root:
        cmd.append('-u')
        cmd.append(f'{os.getuid()}:{os.getgid()}')
    else:
        cmd.append('-u')
        cmd.append('mysql')
    if 'mysql' == clp_config.database.type:
        cmd.append('mysql:8.0.23')
    elif 'mariadb' == clp_config.database.type:
        cmd.append('mariadb:10.6.4-focal')
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    if not wait_for_database_to_init(container_name, clp_config, 150):
        raise EnvironmentError(f"{component_name} did not initialize in time")

    logger.info(f"Started {component_name}.")


def create_db_tables(instance_id: str, clp_config: CLPConfig, container_clp_config: CLPConfig, mounts: CLPDockerMounts):
    component_name = DB_COMPONENT_NAME
    logger.info(f"Creating {component_name} tables...")

    container_name = f'clp-{component_name}-table-creator-{instance_id}'

    # Create database config file
    db_config_filename = f'{container_name}.yml'
    db_config_file_path = clp_config.logs_directory / db_config_filename
    with open(db_config_file_path, 'w') as f:
        yaml.safe_dump(container_clp_config.database.dict(), f)

    clp_site_packages_dir = CONTAINER_CLP_HOME / 'lib' / 'python3' / 'site-packages'
    container_start_cmd = [
        'docker', 'run',
        '-i',
        '--network', 'host',
        '--rm',
        '--name', container_name,
        '-e', f'PYTHONPATH={clp_site_packages_dir}',
        '-u', f'{os.getuid()}:{os.getgid()}',
        '--mount', str(mounts.clp_home),
    ]
    necessary_mounts = [mounts.data_dir, mounts.logs_dir]
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append('--mount')
            container_start_cmd.append(str(mount))
    container_start_cmd.append(clp_config.execution_container)

    clp_py_utils_dir = clp_site_packages_dir / 'clp_py_utils'
    create_tables_cmd = [
        'python3',
        str(clp_py_utils_dir / 'create-db-tables.py'),
        '--config', str(container_clp_config.logs_directory / db_config_filename),
    ]

    cmd = container_start_cmd + create_tables_cmd
    logger.debug(' '.join(cmd))
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    db_config_file_path.unlink()

    logger.info(f"Created {component_name} tables.")


def start_compression_queue(instance_id: str, clp_config: CLPConfig):
    component_name = COMPRESSION_QUEUE_COMPONENT_NAME
    queue_logs_dir = clp_config.logs_directory / component_name
    start_queue(component_name, instance_id, clp_config.compression_queue, queue_logs_dir)


def start_search_queue(instance_id: str, clp_config: CLPConfig):
    component_name = SEARCH_QUEUE_COMPONENT_NAME
    queue_logs_dir = clp_config.logs_directory / component_name
    start_queue(component_name, instance_id, clp_config.search_queue, queue_logs_dir)


def start_queue(component_name: str, instance_id: str, queue_config: Queue,
                queue_logs_dir: pathlib.Path):
    logger.info(f"Starting {component_name}...")

    container_name = f'clp-{component_name}-{instance_id}'
    if container_exists(container_name):
        logger.info(f"{component_name} already running.")
        return

    validate_queue_config(component_name, queue_config, queue_logs_dir)

    # Create directories
    queue_logs_dir.mkdir(exist_ok=True, parents=True)

    is_root = os.getuid() == 0
    if is_root:
        # Make the directories world-writable so that the default user in the
        # container can write to them
        # TODO Find a less brute-force solution
        queue_logs_dir.chmod(0o777)
        queue_logs_dir.chmod(0o777)

    log_filename = 'rabbitmq.log'

    # Generate config file
    config_filename = f'{container_name}.conf'
    host_config_file_path = queue_logs_dir / config_filename
    with open(host_config_file_path, 'w') as f:
        f.write(f"default_user = {queue_config.username}\n")
        f.write(f"default_pass = {queue_config.password}\n")
        f.write(f"log.file = {log_filename}\n")

    # Start container
    rabbitmq_logs_dir = pathlib.Path('/') / 'var' / 'log' / 'rabbitmq'
    mounts = [
        DockerMount(DockerMountType.BIND, host_config_file_path,
                    pathlib.Path('/') / 'etc' / 'rabbitmq' / 'rabbitmq.conf', True),
        DockerMount(DockerMountType.BIND, queue_logs_dir, rabbitmq_logs_dir),
    ]
    rabbitmq_pid_file_path = pathlib.Path('/') / 'tmp' / 'rabbitmq.pid'
    cmd = [
        'docker', 'run',
        '-d',
        '--name', container_name,
        # Override RABBITMQ_LOGS since the image sets it to *only* log to stdout
        '-e', f'RABBITMQ_LOGS={rabbitmq_logs_dir / log_filename}',
        '-e', f'RABBITMQ_PID_FILE={rabbitmq_pid_file_path}',
    ]
    append_docker_port_settings_for_host_ips(queue_config.host, queue_config.port, 5672, cmd)
    for mount in mounts:
        cmd.append('--mount')
        cmd.append(str(mount))
    if not is_root:
        cmd.append('-u')
        cmd.append(f'{os.getuid()}:{os.getgid()}')
    else:
        cmd.append('-u')
        cmd.append('rabbitmq')
    cmd.append('rabbitmq:3.9.8')
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    # Wait for queue to start up
    cmd = [
        'docker', 'exec', '-it', container_name,
        'rabbitmqctl', 'wait', str(rabbitmq_pid_file_path),
    ]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {component_name}.")


def start_results_cache(instance_id: str, clp_config: CLPConfig, conf_dir: pathlib.Path):
    component_name = RESULTS_CACHE_COMPONENT_NAME

    logger.info(f"Starting {component_name}...")

    container_name = f'clp-{component_name}-{instance_id}'
    if is_container_running(container_name):
        logger.info(f"{component_name} already running.")
        return
    elif container_exists(container_name):
        logger.warning(f"{component_name} stopped but not removed.")
        logger.warning(f"run docker rm {container_name} to remove the stale container")
        return

    data_dir = clp_config.data_directory / component_name
    logs_dir = clp_config.logs_directory / component_name

    validate_results_cache_config(clp_config, data_dir, logs_dir)

    # Create directories
    data_dir.mkdir(exist_ok=True, parents=True)
    logs_dir.mkdir(exist_ok=True, parents=True)

    is_root = os.getuid() == 0
    if is_root:
        # Make the directories world-writable so that the default user in the
        # container can write to them
        # TODO Find a less brute-force solution
        data_dir.chmod(0o777)
        logs_dir.chmod(0o777)

    # Start container
    mounts = [
        DockerMount(DockerMountType.BIND, conf_dir / 'mongo', pathlib.Path('/') / 'etc' / 'mongo',
                    True),
        DockerMount(DockerMountType.BIND, data_dir, pathlib.Path('/') / 'data' / 'db'),
        DockerMount(DockerMountType.BIND, logs_dir, pathlib.Path('/') / 'var' / 'log' / 'mongodb'),
    ]
    cmd = [
        'docker', 'run',
        '-d',
        '--name', container_name,
    ]
    for mount in mounts:
        cmd.append('--mount')
        cmd.append(str(mount))
    append_docker_port_settings_for_host_ips(clp_config.results_cache.host,
                                             clp_config.results_cache.port, 27017, cmd)
    if not is_root:
        cmd.append('-u')
        cmd.append(f'{os.getuid()}:{os.getgid()}')
    else:
        cmd.append('-u')
        cmd.append('mongodb')
    cmd.append('mongo:7.0.1')
    cmd.append('--config')
    cmd.append(str(pathlib.Path('/') / 'etc' / 'mongo' / 'mongod.conf'))
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {component_name}.")


def start_compression_job_handler(instance_id: str, clp_config: CLPConfig, container_clp_config: CLPConfig,
                                  mounts: CLPDockerMounts):
    component_name = COMPRESSION_JOB_HANDLER_COMPONENT_NAME

    logger.info(f"Starting {component_name}...")

    container_name = f'clp-{component_name}-{instance_id}'
    if container_exists(container_name):
        logger.info(f"{component_name} already running.")
        return

    container_config_filename = f'{container_name}.yml'
    container_config_file_path = clp_config.logs_directory / container_config_filename
    with open(container_config_file_path, 'w') as f:
        yaml.safe_dump(container_clp_config.dump_to_primitive_dict(), f)

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)
    container_logs_dir = container_clp_config.logs_directory / component_name

    clp_config.archive_output.directory.mkdir(parents=True, exist_ok=True)

    clp_site_packages_dir = CONTAINER_CLP_HOME / 'lib' / 'python3' / 'site-packages'
    container_start_cmd = [
        'docker', 'run',
        '-di',
        '--network', 'host',
        '-w', str(CONTAINER_CLP_HOME),
        '--name', container_name,
        '-e', f'PYTHONPATH={clp_site_packages_dir}',
        '-e', f'BROKER_URL=amqp://'
              f'{container_clp_config.compression_queue.username}:{container_clp_config.compression_queue.password}@'
              f'{container_clp_config.compression_queue.host}:{container_clp_config.compression_queue.port}',
        '-e', f'RESULT_BACKEND=rpc://'
              f'{container_clp_config.compression_queue.username}:{container_clp_config.compression_queue.password}'
              f'@{container_clp_config.compression_queue.host}:{container_clp_config.compression_queue.port}',
        '-e', f'CLP_LOGS_DIR={container_logs_dir}',
        '-e', f'CLP_LOGGING_LEVEL={clp_config.compression_job_handler.logging_level}',
        # V0.5 TODO: customization for filer
        #'-u', f'{os.getuid()}:{os.getgid()}',
        '-u', f'root:root',
        '--mount', str(mounts.clp_home),
    ]
    necessary_mounts = [
        mounts.archives_output_dir,
        mounts.input_logs_dir,
        mounts.logs_dir,
    ]
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append('--mount')
            container_start_cmd.append(str(mount))
    container_start_cmd.append(clp_config.execution_container)

    scheduler_cmd = [
        'python3', '-u', '-m',
        'compression_job_handler.main',
        '--config', str(container_clp_config.logs_directory / container_config_filename),
        '--no-progress-reporting',
    ]
    cmd = container_start_cmd + scheduler_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)
    logger.info(f"Started {component_name}.")


def start_search_scheduler(instance_id: str, clp_config: CLPConfig, container_clp_config: CLPConfig,
                           mounts: CLPDockerMounts):
    component_name = SEARCH_SCHEDULER_COMPONENT_NAME
    logger.info(f"Starting {component_name}...")

    container_name = f'clp-{component_name}-{instance_id}'
    if container_exists(container_name):
        logger.info(f"{SEARCH_SCHEDULER_COMPONENT_NAME} already running.")
        return

    container_config_filename = f'{container_name}.yml'
    container_config_file_path = clp_config.logs_directory / container_config_filename
    with open(container_config_file_path, 'w') as f:
        yaml.safe_dump(container_clp_config.dump_to_primitive_dict(), f)

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)
    container_logs_dir = container_clp_config.logs_directory / component_name

    clp_site_packages_dir = CONTAINER_CLP_HOME / 'lib' / 'python3' / 'site-packages'
    container_start_cmd = [
        'docker', 'run',
        '-di',
        '--network', 'host',
        '-w', str(CONTAINER_CLP_HOME),
        '--name', container_name,
        '-e', f'PYTHONPATH={clp_site_packages_dir}',
        '-e', f'BROKER_URL=amqp://'
              f'{container_clp_config.search_queue.username}:{container_clp_config.search_queue.password}@'
              f'{container_clp_config.search_queue.host}:{container_clp_config.search_queue.port}',
        '-e', f'RESULT_BACKEND=rpc://'
              f'{container_clp_config.search_queue.username}:{container_clp_config.search_queue.password}'
              f'@{container_clp_config.search_queue.host}:{container_clp_config.search_queue.port}',
        '-e', f'CLP_LOGS_DIR={container_logs_dir}',
        '-e', f'CLP_LOGGING_LEVEL={clp_config.search_scheduler.logging_level}',
        # V0.5 TODO: customization for filer
        #'-u', f'{os.getuid()}:{os.getgid()}',
        '-u', f'root:root',
        '--mount', str(mounts.clp_home),
    ]
    necessary_mounts = [
        mounts.logs_dir,
    ]
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append('--mount')
            container_start_cmd.append(str(mount))
    container_start_cmd.append(clp_config.execution_container)
    scheduler_cmd = [
        'python3', '-u', '-m',
        'job_orchestration.scheduler.search_scheduler',
        '--config', str(container_clp_config.logs_directory / container_config_filename),
    ]

    # for now, let's use what we have.
    cmd = container_start_cmd + scheduler_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {component_name}.")


def start_compression_worker(instance_id: str, clp_config: CLPConfig, container_clp_config: CLPConfig,
                             num_cpus: int, mounts: CLPDockerMounts):
    celery_method = 'job_orchestration.executor.compression'
    celery_route = f"{QueueName.COMPRESSION}"
    start_worker(
        COMPRESSION_WORKER_COMPONENT_NAME,
        instance_id,
        clp_config,
        clp_config.compression_worker,
        container_clp_config,
        container_clp_config.compression_queue,
        celery_method,
        celery_route,
        num_cpus,
        mounts
    )


def start_search_worker(instance_id: str, clp_config: CLPConfig, container_clp_config: CLPConfig,
                        num_cpus: int, mounts: CLPDockerMounts):
    celery_method = 'job_orchestration.executor.search'
    celery_route = f"{QueueName.SEARCH}"
    start_worker(
        SEARCH_WORKER_COMPONENT_NAME,
        instance_id,
        clp_config,
        clp_config.search_worker,
        container_clp_config,
        container_clp_config.search_queue,
        celery_method,
        celery_route,
        num_cpus,
        mounts
    )


def start_worker(component_name: str, instance_id: str, clp_config: CLPConfig, worker_config: BaseModel,
                 container_clp_config: CLPConfig, queue_config: Queue, celery_method: str,
                 celery_route: str, num_cpus: int, mounts: CLPDockerMounts):
    logger.info(f"Starting {component_name}...")

    container_name = f'clp-{component_name}-{instance_id}'
    if container_exists(container_name):
        logger.info(f"{component_name} already running.")
        return

    validate_worker_config(clp_config)

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)
    container_logs_dir = container_clp_config.logs_directory / component_name

    # Create necessary directories
    clp_config.archive_output.directory.mkdir(parents=True, exist_ok=True)

    clp_site_packages_dir = CONTAINER_CLP_HOME / 'lib' / 'python3' / 'site-packages'
    container_start_cmd = [
        'docker', 'run',
        '-di',
        '--network', 'host',
        '-w', str(CONTAINER_CLP_HOME),
        '--name', container_name,
        '-e', f'PYTHONPATH={clp_site_packages_dir}',
        '-e', f'BROKER_URL=amqp://'
              f'{queue_config.username}:{queue_config.password}@'
              f'{queue_config.host}:{queue_config.port}',
        '-e', f'RESULT_BACKEND=rpc://'
              f'{queue_config.username}:{queue_config.password}'
              f'@{queue_config.host}:{queue_config.port}',
        '-e', f'CLP_HOME={CONTAINER_CLP_HOME}',
        '-e', f'CLP_DATA_DIR={container_clp_config.data_directory}',
        '-e', f'CLP_ARCHIVE_OUTPUT_DIR={container_clp_config.archive_output.directory}',
        '-e', f'CLP_LOGS_DIR={container_logs_dir}',
        '-e', f'CLP_LOGGING_LEVEL={worker_config.logging_level}',
        # V0.5 TODO: customization for filer
        #'-u', f'{os.getuid()}:{os.getgid()}',
        '-u', f'root:root',
        '--mount', str(mounts.clp_home),
    ]
    necessary_mounts = [
        mounts.data_dir,
        mounts.logs_dir,
        mounts.archives_output_dir,
        mounts.input_logs_dir,
    ]
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append('--mount')
            container_start_cmd.append(str(mount))
    container_start_cmd.append(clp_config.execution_container)

    worker_cmd = [
        str(clp_site_packages_dir / 'bin' / 'celery'),
        '-A',
        celery_method,
        'worker',
        '--concurrency', str(num_cpus),
        '--loglevel', 'WARNING',
        '-f', str(container_logs_dir / "worker.log"),
        '-Q', celery_route,
        '-n', component_name,
    ]
    cmd = container_start_cmd + worker_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {component_name}.")

def get_host_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(0)
    try:
        # doesn't even have to be reachable
        s.connect(('10.254.254.254', 1))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

def start_reducer(instance_id: str, clp_config: CLPConfig, container_clp_config: CLPConfig,
                  num_cpus: int, mounts: CLPDockerMounts):
    component_name = REDUCER_COMPONENT_NAME
    logger.info(f"Starting {component_name}...")

    container_name = f'clp-{component_name}-{instance_id}'
    if container_exists(container_name):
        logger.info(f"{component_name} already running.")
        return

    container_config_filename = f'{container_name}.yml'
    container_config_file_path = clp_config.logs_directory / container_config_filename
    with open(container_config_file_path, 'w') as f:
        yaml.safe_dump(container_clp_config.dump_to_primitive_dict(), f)

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)
    container_logs_dir = container_clp_config.logs_directory / component_name

    clp_site_packages_dir = CONTAINER_CLP_HOME / 'lib' / 'python3' / 'site-packages'
    container_start_cmd = [
        'docker', 'run',
        '-di',
        '--network', 'host',
        '-w', str(CONTAINER_CLP_HOME),
        '--name', container_name,
        '-e', f'PYTHONPATH={clp_site_packages_dir}',
        '-e', f'CLP_LOGS_DIR={container_logs_dir}',
        '-e', f'CLP_LOGGING_LEVEL={clp_config.reducer.logging_level}',
        '-e', f'CLP_HOME={CONTAINER_CLP_HOME}',
        '--mount', str(mounts.clp_home),
    ]
    necessary_mounts = [
        mounts.logs_dir,
    ]
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append('--mount')
            container_start_cmd.append(str(mount))
    container_start_cmd.append(clp_config.execution_container)

    reducer_cmd = [
        'python3', '-u', '-m',
        'job_orchestration.reducer.reducer',
        '--config', str(container_clp_config.logs_directory / container_config_filename),
        '--host', get_host_ip(),
        "--concurrency", str(num_cpus),
        "--polling-interval-ms", str(clp_config.reducer.polling_interval),
    ]

    cmd = container_start_cmd + reducer_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {component_name}")

def start_webui(instance_id: str, clp_config: CLPConfig, mounts: CLPDockerMounts):
    component_name = WEBUI_COMPONENT_NAME

    logger.info(f"Starting {component_name}...")

    container_name = f'clp-{component_name}-{instance_id}'
    if container_exists(container_name):
        logger.info(f"{component_name} already running.")
        return

    validate_webui_config(clp_config)

    settings = {
        "public": {
            "SearchResultsCollectionName": clp_config.results_cache.results_collection_name,
            "SearchResultsMetadataCollectionName": clp_config.results_cache.metadata_collection_name,
            # TODO Hard-coded
            "StatsCollectionName": "stats",
        },
        "private": {
            "QueryHandlerHost": clp_config.webui_query_handler.host,
            "QueryHandlerWebsocket": clp_config.webui_query_handler.port,
        }
    }

    # Start container
    container_cmd = [
        'docker', 'run',
        '-d',
        '--network', 'host',
        '--name', container_name,
        '-u', f'{os.getuid()}:{os.getgid()}',
        '-e', f"MONGO_URL={clp_config.results_cache.get_uri()}",
        '-e', f"PORT={clp_config.webui.port}",
        '-e', f"ROOT_URL=http://{clp_config.webui.host}",
        '-e', f"METEOR_SETTINGS={json.dumps(settings)}",
    ]
    necessary_mounts = [
        mounts.clp_home,
    ]
    for mount in necessary_mounts:
        if mount:
            container_cmd.append('--mount')
            container_cmd.append(str(mount))
    container_cmd.append(clp_config.execution_container)

    node_cmd = [
        str(CONTAINER_CLP_HOME / 'bin' / 'node'),
        str(CONTAINER_CLP_HOME / 'var' / 'www' / 'main.js')
    ]
    cmd = container_cmd + node_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {component_name}.")


def start_webui_query_handler(instance_id: str, clp_config: CLPConfig,
                              container_clp_config: CLPConfig, mounts: CLPDockerMounts):
    component_name = WEBUI_QUERY_HANDLER_COMPONENT_NAME

    logger.info(f"Starting {component_name}...")

    container_name = f'clp-{component_name}-{instance_id}'
    if container_exists(container_name):
        logger.info(f"{component_name} already running.")
        return

    validate_webui_query_handler_config(clp_config)

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)
    container_logs_dir = container_clp_config.logs_directory / component_name

    # Create necessary directories
    clp_config.archive_output.directory.mkdir(parents=True, exist_ok=True)

    clp_site_packages_dir = CONTAINER_CLP_HOME / 'lib' / 'python3' / 'site-packages'
    container_start_cmd = [
        'docker', 'run',
        '-di',
        '--network', 'host',
        '-w', str(CONTAINER_CLP_HOME),
        '--name', container_name,
        '-e', f'PYTHONPATH={clp_site_packages_dir}',
        '-e', f'CLP_DB_USER={clp_config.database.username}',
        '-e', f'CLP_DB_PASS={clp_config.database.password}',
        '-e', f'CLP_LOGS_DIR={container_logs_dir}',
        '-e', f'CLP_LOGGING_LEVEL={clp_config.webui_query_handler.logging_level}',
        '-u', f'{os.getuid()}:{os.getgid()}',
        '--mount', str(mounts.clp_home),
    ]
    necessary_mounts = [
        mounts.data_dir,
        mounts.logs_dir,
        mounts.archives_output_dir,
        mounts.input_logs_dir,
    ]
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append('--mount')
            container_start_cmd.append(str(mount))
    container_start_cmd.append(clp_config.execution_container)

    query_handler_cmd = [
        "python3",
        "-u",
        "-m",
        "webui_query_handler.main",
        "--host", str(clp_config.webui_query_handler.host),
        "--port", str(clp_config.webui_query_handler.port),
        "--db-host", str(clp_config.database.host),
        "--db-port", str(clp_config.database.port),
        "--db-name", str(clp_config.database.name),
        "--results-cache-uri", f"{clp_config.results_cache.get_uri()}",
        "--results-cache-results-collection", f"{clp_config.results_cache.results_collection_name}",
        "--results-cache-metadata-collection",
        f"{clp_config.results_cache.metadata_collection_name}",
    ]
    cmd = container_start_cmd + query_handler_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {component_name}.")


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Starts CLP")
    args_parser.add_argument('--config', '-c', default=str(default_config_file_path),
                             help="CLP package configuration file.")

    # Constant
    DATABASE_COMPONENTS = "databases"
    CONTROLLER_COMPONENTS = "controllers"

    component_args_parser = args_parser.add_subparsers(dest='component_name')
    component_args_parser.add_parser(DB_COMPONENT_NAME)
    component_args_parser.add_parser(COMPRESSION_QUEUE_COMPONENT_NAME)
    component_args_parser.add_parser(RESULTS_CACHE_COMPONENT_NAME)
    component_args_parser.add_parser(SEARCH_QUEUE_COMPONENT_NAME)
    component_args_parser.add_parser(SEARCH_SCHEDULER_COMPONENT_NAME)
    component_args_parser.add_parser(COMPRESSION_JOB_HANDLER_COMPONENT_NAME)
    # need to think about how to specify the argument
    component_args_parser.add_parser(SEARCH_WORKER_COMPONENT_NAME)
    component_args_parser.add_parser(COMPRESSION_WORKER_COMPONENT_NAME)
    component_args_parser.add_parser(WEBUI_COMPONENT_NAME)
    component_args_parser.add_parser(WEBUI_QUERY_HANDLER_COMPONENT_NAME)
    component_args_parser.add_parser(REDUCER_COMPONENT_NAME)
    # Shortcut for multiple components
    component_args_parser.add_parser(DATABASE_COMPONENTS)
    component_args_parser.add_parser(CONTROLLER_COMPONENTS)
    # V0.5 TODO properly handle num-cpus argument
    # worker_args_parser = component_args_parser.add_parser(COMPRESSION_WORKER_COMPONENT_NAME)
    args_parser.add_argument('--num-cpus', type=int, default=0,
                             help="Number of logical CPU cores to use for compression")

    parsed_args = args_parser.parse_args(argv[1:])

    if parsed_args.component_name:
        component_name = parsed_args.component_name
    else:
        component_name = ""

    try:
        check_dependencies()
    except:
        logger.exception("Dependency checking failed.")
        return -1

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = validate_and_load_config_file(config_file_path, default_config_file_path, clp_home)

        # Validate and load necessary credentials
        if component_name in ['', DB_COMPONENT_NAME, SEARCH_SCHEDULER_COMPONENT_NAME,
                              COMPRESSION_JOB_HANDLER_COMPONENT_NAME,
                              WEBUI_QUERY_HANDLER_COMPONENT_NAME, DATABASE_COMPONENTS,
                              CONTROLLER_COMPONENTS, REDUCER_COMPONENT_NAME]:
            validate_and_load_db_credentials_file(clp_config, clp_home, True)
        if component_name in [
            '',
            COMPRESSION_QUEUE_COMPONENT_NAME,
            SEARCH_SCHEDULER_COMPONENT_NAME,
            SEARCH_QUEUE_COMPONENT_NAME,
            COMPRESSION_JOB_HANDLER_COMPONENT_NAME,
            COMPRESSION_WORKER_COMPONENT_NAME,
            SEARCH_WORKER_COMPONENT_NAME,
            CONTROLLER_COMPONENTS
        ]:
            # TODO Validate the search queue credentials when the search queue is actually used
            validate_and_load_compression_queue_credentials_file(clp_config, clp_home, True)

        clp_config.validate_data_dir()
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    # Get the number of CPU cores to use
    num_cpus = multiprocessing.cpu_count()
    if '' == component_name:
        if parsed_args.num_cpus != 0:
            num_cpus = parsed_args.num_cpus
        num_cpus = int(num_cpus / 2)

    if COMPRESSION_WORKER_COMPONENT_NAME == component_name and parsed_args.num_cpus != 0:
        num_cpus = parsed_args.num_cpus
    if SEARCH_WORKER_COMPONENT_NAME == component_name and parsed_args.num_cpus != 0:
        num_cpus = parsed_args.num_cpus
    if REDUCER_COMPONENT_NAME == component_name and parsed_args.num_cpus != 0:
        num_cpus = parsed_args.num_cpus

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)

    # Create necessary directories
    clp_config.data_directory.mkdir(parents=True, exist_ok=True)
    clp_config.logs_directory.mkdir(parents=True, exist_ok=True)

    try:
        # Create instance-id file
        instance_id_file_path = clp_config.logs_directory / 'instance-id'
        if instance_id_file_path.exists():
            with open(instance_id_file_path, 'r') as f:
                instance_id = f.readline()
        else:
            instance_id = str(uuid.uuid4())[-4:]
            with open(instance_id_file_path, 'w') as f:
                f.write(instance_id)
                f.flush()

        conf_dir = clp_home / 'etc'

        # Start components
        if component_name in ['', DB_COMPONENT_NAME, DATABASE_COMPONENTS]:
            start_db(instance_id, clp_config, conf_dir)
            create_db_tables(instance_id, clp_config, container_clp_config, mounts)
        if component_name in ['', RESULTS_CACHE_COMPONENT_NAME, DATABASE_COMPONENTS]:
            start_results_cache(instance_id, clp_config, conf_dir)
        if component_name in ['', COMPRESSION_QUEUE_COMPONENT_NAME, CONTROLLER_COMPONENTS]:
            start_compression_queue(instance_id, clp_config)
        if component_name in ['', SEARCH_QUEUE_COMPONENT_NAME, CONTROLLER_COMPONENTS]:
            start_search_queue(instance_id, clp_config)
        if component_name in ['', COMPRESSION_JOB_HANDLER_COMPONENT_NAME, CONTROLLER_COMPONENTS]:
            start_compression_job_handler(instance_id, clp_config, container_clp_config, mounts)
        if component_name in ['', SEARCH_SCHEDULER_COMPONENT_NAME, CONTROLLER_COMPONENTS]:
            start_search_scheduler(instance_id, clp_config, container_clp_config, mounts)
        if component_name in ['', SEARCH_WORKER_COMPONENT_NAME]:
            start_search_worker(instance_id, clp_config, container_clp_config, num_cpus, mounts)
        if component_name in ['', REDUCER_COMPONENT_NAME]:
            start_reducer(instance_id, clp_config, container_clp_config, num_cpus, mounts)
        if component_name in ['', COMPRESSION_WORKER_COMPONENT_NAME]:
            start_compression_worker(instance_id, clp_config, container_clp_config, num_cpus, mounts)
        if component_name in ['', WEBUI_QUERY_HANDLER_COMPONENT_NAME]:
            start_webui_query_handler(instance_id, clp_config, container_clp_config, mounts)
        if component_name in ['', WEBUI_COMPONENT_NAME]:
            start_webui(instance_id, clp_config, mounts)
    except Exception as ex:
        if type(ex) == ValueError:
            logger.error(f"Failed to start CLP: {ex}")
        else:
            logger.exception("Failed to start CLP.")

        return -1

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))
