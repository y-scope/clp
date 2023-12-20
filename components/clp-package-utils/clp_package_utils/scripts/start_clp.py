import argparse
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

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CONTAINER_CLP_HOME,
    DB_COMPONENT_NAME,
    QUEUE_COMPONENT_NAME,
    SCHEDULER_COMPONENT_NAME,
    WORKER_COMPONENT_NAME,
    check_dependencies,
    container_exists,
    CLPDockerMounts,
    DockerMount,
    DockerMountType,
    generate_container_config,
    get_clp_home,
    validate_and_load_config_file,
    validate_and_load_db_credentials_file,
    validate_and_load_queue_credentials_file,
    validate_db_config,
    validate_queue_config,
    validate_worker_config
)
from clp_py_utils.clp_config import CLPConfig
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

    logger.error("Timeout while waiting for database to initialize.")
    return False


def start_db(instance_id: str, clp_config: CLPConfig, conf_dir: pathlib.Path):
    logger.info("Starting database...")

    container_name = f'clp-{DB_COMPONENT_NAME}-{instance_id}'
    if container_exists(container_name):
        logger.info("Database already running.")
        return

    db_data_dir = clp_config.data_directory / DB_COMPONENT_NAME
    db_logs_dir = clp_config.logs_directory / DB_COMPONENT_NAME

    validate_db_config(clp_config, db_data_dir, db_logs_dir)

    # Create directories
    db_data_dir.mkdir(exist_ok=True, parents=True)
    db_logs_dir.mkdir(exist_ok=True, parents=True)

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
        '--rm',
        '--name', container_name,
        '-e', f'MYSQL_ROOT_PASSWORD={clp_config.database.password}',
        '-e', f'MYSQL_USER={clp_config.database.username}',
        '-e', f'MYSQL_PASSWORD={clp_config.database.password}',
        '-e', f'MYSQL_DATABASE={clp_config.database.name}',
        '-u', f'{os.getuid()}:{os.getgid()}',
    ]
    for mount in mounts:
        cmd.append('--mount')
        cmd.append(str(mount))
    append_docker_port_settings_for_host_ips(clp_config.database.host, clp_config.database.port, 3306, cmd)
    if 'mysql' == clp_config.database.type:
        cmd.append('mysql:8.0.23')
    elif 'mariadb' == clp_config.database.type:
        cmd.append('mariadb:10.6.4-focal')
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    if not wait_for_database_to_init(container_name, clp_config, 30):
        raise EnvironmentError("Database did not initialize in time")

    logger.info("Started database.")


def create_db_tables(instance_id: str, clp_config: CLPConfig, container_clp_config: CLPConfig, mounts: CLPDockerMounts):
    logger.info("Creating database tables...")

    container_name = f'clp-db-table-creator-{instance_id}'

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

    logger.info("Created database tables.")


def start_queue(instance_id: str, clp_config: CLPConfig):
    logger.info("Starting queue...")

    container_name = f'clp-{QUEUE_COMPONENT_NAME}-{instance_id}'
    if container_exists(container_name):
        logger.info("Queue already running.")
        return

    queue_logs_dir = clp_config.logs_directory / QUEUE_COMPONENT_NAME
    validate_queue_config(clp_config, queue_logs_dir)

    log_filename = 'rabbitmq.log'

    # Generate config file
    config_filename = f'{container_name}.conf'
    host_config_file_path = clp_config.logs_directory / config_filename
    with open(host_config_file_path, 'w') as f:
        f.write(f"default_user = {clp_config.queue.username}\n")
        f.write(f"default_pass = {clp_config.queue.password}\n")
        f.write(f"log.file = {log_filename}\n")

    # Create directories
    queue_logs_dir.mkdir(exist_ok=True, parents=True)

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
        '--rm',
        '--name', container_name,
        # Override RABBITMQ_LOGS since the image sets it to *only* log to stdout
        '-e', f'RABBITMQ_LOGS={rabbitmq_logs_dir / log_filename}',
        '-e', f'RABBITMQ_PID_FILE={rabbitmq_pid_file_path}',
        '-u', f'{os.getuid()}:{os.getgid()}',
    ]
    append_docker_port_settings_for_host_ips(clp_config.queue.host, clp_config.queue.port, 5672, cmd)
    for mount in mounts:
        cmd.append('--mount')
        cmd.append(str(mount))
    cmd.append('rabbitmq:3.9.8')
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    # Wait for queue to start up
    cmd = [
        'docker', 'exec', '-it', container_name,
        'rabbitmqctl', 'wait', str(rabbitmq_pid_file_path),
    ]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info("Started queue.")


def start_scheduler(instance_id: str, clp_config: CLPConfig, container_clp_config: CLPConfig, mounts: CLPDockerMounts):
    logger.info("Starting scheduler...")

    container_name = f'clp-{SCHEDULER_COMPONENT_NAME}-{instance_id}'
    if container_exists(container_name):
        logger.info("Scheduler already running.")
        return

    container_config_filename = f'{container_name}.yml'
    container_config_file_path = clp_config.logs_directory / container_config_filename
    with open(container_config_file_path, 'w') as f:
        yaml.safe_dump(container_clp_config.dump_to_primitive_dict(), f)

    clp_site_packages_dir = CONTAINER_CLP_HOME / 'lib' / 'python3' / 'site-packages'
    container_start_cmd = [
        'docker', 'run',
        '-di',
        '--network', 'host',
        '-w', str(CONTAINER_CLP_HOME),
        '--rm',
        '--name', container_name,
        '-e', f'PYTHONPATH={clp_site_packages_dir}',
        '-e', f'BROKER_URL=amqp://'
              f'{container_clp_config.queue.username}:{container_clp_config.queue.password}@'
              f'{container_clp_config.queue.host}:{container_clp_config.queue.port}',
        '-u', f'{os.getuid()}:{os.getgid()}',
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
        'job_orchestration.scheduler.scheduler',
        '--config', str(container_clp_config.logs_directory / container_config_filename),
    ]
    cmd = container_start_cmd + scheduler_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info("Started scheduler.")


def start_worker(instance_id: str, clp_config: CLPConfig, container_clp_config: CLPConfig, num_cpus: int,
                 mounts: CLPDockerMounts):
    logger.info("Starting worker...")

    container_name = f'clp-{WORKER_COMPONENT_NAME}-{instance_id}'
    if container_exists(container_name):
        logger.info("Worker already running.")
        return

    validate_worker_config(clp_config)

    # Create necessary directories
    clp_config.archive_output.directory.mkdir(parents=True, exist_ok=True)

    clp_site_packages_dir = CONTAINER_CLP_HOME / 'lib' / 'python3' / 'site-packages'
    container_start_cmd = [
        'docker', 'run',
        '-di',
        '--network', 'host',
        '-w', str(CONTAINER_CLP_HOME),
        '--rm',
        '--name', container_name,
        '-e', f'PYTHONPATH={clp_site_packages_dir}',
        '-e', f'BROKER_URL=amqp://'
              f'{container_clp_config.queue.username}:{container_clp_config.queue.password}@'
              f'{container_clp_config.queue.host}:{container_clp_config.queue.port}',
        '-e', f'RESULT_BACKEND=rpc://'
              f'{container_clp_config.queue.username}:{container_clp_config.queue.password}'
              f'@{container_clp_config.queue.host}:{container_clp_config.queue.port}',
        '-e', f'CLP_HOME={CONTAINER_CLP_HOME}',
        '-e', f'CLP_DATA_DIR={container_clp_config.data_directory}',
        '-e', f'CLP_ARCHIVE_OUTPUT_DIR={container_clp_config.archive_output.directory}',
        '-e', f'CLP_LOGS_DIR={container_clp_config.logs_directory}',
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

    worker_cmd = [
        str(clp_site_packages_dir / 'bin' / 'celery'),
        '-A',
        'job_orchestration.executor',
        'worker',
        '--concurrency', str(num_cpus),
        '--loglevel', 'WARNING',
        '-Q', f"{QueueName.COMPRESSION},{QueueName.SEARCH}",
    ]
    cmd = container_start_cmd + worker_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info("Started worker.")


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Starts CLP")
    args_parser.add_argument('--config', '-c', default=str(default_config_file_path),
                             help="CLP package configuration file.")

    component_args_parser = args_parser.add_subparsers(dest='component_name')
    component_args_parser.add_parser(DB_COMPONENT_NAME)
    component_args_parser.add_parser(QUEUE_COMPONENT_NAME)
    component_args_parser.add_parser(SCHEDULER_COMPONENT_NAME)
    worker_args_parser = component_args_parser.add_parser(WORKER_COMPONENT_NAME)
    worker_args_parser.add_argument('--num-cpus', type=int, default=0,
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
        if component_name in ['', DB_COMPONENT_NAME, SCHEDULER_COMPONENT_NAME]:
            validate_and_load_db_credentials_file(clp_config, clp_home, True)
        if component_name in ['', QUEUE_COMPONENT_NAME, SCHEDULER_COMPONENT_NAME, WORKER_COMPONENT_NAME]:
            validate_and_load_queue_credentials_file(clp_config, clp_home, True)

        clp_config.validate_data_dir()
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    # Get the number of CPU cores to use
    num_cpus = multiprocessing.cpu_count()
    if WORKER_COMPONENT_NAME == component_name and parsed_args.num_cpus != 0:
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
        if '' == component_name or DB_COMPONENT_NAME == component_name:
            start_db(instance_id, clp_config, conf_dir)
            create_db_tables(instance_id, clp_config, container_clp_config, mounts)
        if '' == component_name or QUEUE_COMPONENT_NAME == component_name:
            start_queue(instance_id, clp_config)
        if '' == component_name or SCHEDULER_COMPONENT_NAME == component_name:
            start_scheduler(instance_id, clp_config, container_clp_config, mounts)
        if '' == component_name or WORKER_COMPONENT_NAME == component_name:
            start_worker(instance_id, clp_config, container_clp_config, num_cpus, mounts)
    except Exception as ex:
        # Stop CLP
        subprocess.run(['python3', str(clp_home / 'sbin' / 'stop-clp')], check=True)

        if type(ex) == ValueError:
            logger.error(f"Failed to start CLP: {ex}")
        else:
            logger.exception("Failed to start CLP.")
        return -1

    return 0


if '__main__' == __name__:
    sys.exit(main(sys.argv))
