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
import typing
import uuid

import yaml
from clp_py_utils.clp_config import (
    ALL_TARGET_NAME,
    CLP_METADATA_TABLE_PREFIX,
    CLPConfig,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    CONTROLLER_TARGET_NAME,
    DB_COMPONENT_NAME,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    SEARCH_JOBS_TABLE_NAME,
    SEARCH_SCHEDULER_COMPONENT_NAME,
    SEARCH_WORKER_COMPONENT_NAME,
    WEBUI_COMPONENT_NAME,
)
from job_orchestration.scheduler.constants import QueueName
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
    validate_and_load_config_file,
    validate_and_load_db_credentials_file,
    validate_and_load_queue_credentials_file,
    validate_and_load_redis_credentials_file,
    validate_db_config,
    validate_queue_config,
    validate_redis_config,
    validate_results_cache_config,
    validate_webui_config,
    validate_worker_config,
)

# Setup logging
# Create logger
logger = logging.getLogger("clp")
logger.setLevel(logging.INFO)
# Setup console logging
logging_console_handler = logging.StreamHandler()
logging_formatter = logging.Formatter("%(asctime)s [%(levelname)s] [%(name)s] %(message)s")
logging_console_handler.setFormatter(logging_formatter)
logger.addHandler(logging_console_handler)


def append_docker_port_settings_for_host_ips(
    hostname: str, host_port: int, container_port: int, cmd: [str]
):
    # Note: We use a set because gethostbyname_ex can return the same IP twice for one hostname
    for ip in set(socket.gethostbyname_ex(hostname)[2]):
        cmd.append("-p")
        cmd.append(f"{ip}:{host_port}:{container_port}")


def wait_for_container_cmd(container_name: str, cmd_to_run: [str], timeout: int):
    container_exec_cmd = ["docker", "exec", container_name]
    cmd = container_exec_cmd + cmd_to_run

    begin_time = time.time()

    while True:
        try:
            subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
            return True
        except subprocess.CalledProcessError:
            if time.time() - begin_time > timeout:
                break
            time.sleep(1)

    cmd_str = " ".join(cmd_to_run)
    logger.error(f"Timeout while waiting for command {cmd_str} to run after {timeout} seconds")
    return False


def start_db(instance_id: str, clp_config: CLPConfig, conf_dir: pathlib.Path):
    logger.info(f"Starting {DB_COMPONENT_NAME}...")

    container_name = f"clp-{DB_COMPONENT_NAME}-{instance_id}"
    if container_exists(container_name):
        logger.info(f"{DB_COMPONENT_NAME} already running.")
        return

    db_data_dir = clp_config.data_directory / DB_COMPONENT_NAME
    db_logs_dir = clp_config.logs_directory / DB_COMPONENT_NAME

    validate_db_config(clp_config, db_data_dir, db_logs_dir)

    # Create directories
    db_data_dir.mkdir(exist_ok=True, parents=True)
    db_logs_dir.mkdir(exist_ok=True, parents=True)

    # Start container
    mounts = [
        DockerMount(
            DockerMountType.BIND,
            conf_dir / "mysql" / "conf.d",
            pathlib.Path("/") / "etc" / "mysql" / "conf.d",
            True,
        ),
        DockerMount(DockerMountType.BIND, db_data_dir, pathlib.Path("/") / "var" / "lib" / "mysql"),
        DockerMount(DockerMountType.BIND, db_logs_dir, pathlib.Path("/") / "var" / "log" / "mysql"),
    ]
    # fmt: off
    cmd = [
        "docker", "run",
        "-d",
        "--rm",
        "--name", container_name,
        "-e", f"MYSQL_ROOT_PASSWORD={clp_config.database.password}",
        "-e", f"MYSQL_USER={clp_config.database.username}",
        "-e", f"MYSQL_PASSWORD={clp_config.database.password}",
        "-e", f"MYSQL_DATABASE={clp_config.database.name}",
        "-u", f"{os.getuid()}:{os.getgid()}",
    ]
    # fmt: on
    for mount in mounts:
        cmd.append("--mount")
        cmd.append(str(mount))
    append_docker_port_settings_for_host_ips(
        clp_config.database.host, clp_config.database.port, 3306, cmd
    )
    if "mysql" == clp_config.database.type:
        cmd.append("mysql:8.0.23")
    elif "mariadb" == clp_config.database.type:
        cmd.append("mariadb:10.6.4-focal")
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    # fmt: off
    mysqladmin_cmd = [
        "mysqladmin", "ping",
        "--silent",
        "-h", "127.0.0.1",
        "-u", str(clp_config.database.username),
        f"--password={clp_config.database.password}",
    ]
    # fmt: on

    if not wait_for_container_cmd(container_name, mysqladmin_cmd, 30):
        raise EnvironmentError(f"{DB_COMPONENT_NAME} did not initialize in time")

    logger.info(f"Started {DB_COMPONENT_NAME}.")


def create_db_tables(
    instance_id: str,
    clp_config: CLPConfig,
    container_clp_config: CLPConfig,
    mounts: CLPDockerMounts,
):
    logger.info(f"Creating {DB_COMPONENT_NAME} tables...")

    container_name = f"clp-{DB_COMPONENT_NAME}-table-creator-{instance_id}"

    # Create database config file
    db_config_filename = f"{container_name}.yml"
    db_config_file_path = clp_config.logs_directory / db_config_filename
    with open(db_config_file_path, "w") as f:
        yaml.safe_dump(container_clp_config.database.dict(), f)

    clp_site_packages_dir = CONTAINER_CLP_HOME / "lib" / "python3" / "site-packages"
    # fmt: off
    container_start_cmd = [
        "docker", "run",
        "-i",
        "--network", "host",
        "--rm",
        "--name", container_name,
        "-e", f"PYTHONPATH={clp_site_packages_dir}",
        "-u", f"{os.getuid()}:{os.getgid()}",
        "--mount", str(mounts.clp_home),
    ]
    # fmt: on
    necessary_mounts = [mounts.data_dir, mounts.logs_dir]
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append("--mount")
            container_start_cmd.append(str(mount))
    container_start_cmd.append(clp_config.execution_container)

    clp_py_utils_dir = clp_site_packages_dir / "clp_py_utils"
    # fmt: off
    create_tables_cmd = [
        "python3",
        str(clp_py_utils_dir / "create-db-tables.py"),
        "--config", str(container_clp_config.logs_directory / db_config_filename),
    ]
    # fmt: on

    cmd = container_start_cmd + create_tables_cmd
    logger.debug(" ".join(cmd))
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    db_config_file_path.unlink()

    logger.info(f"Created {DB_COMPONENT_NAME} tables.")


def start_queue(instance_id: str, clp_config: CLPConfig):
    logger.info(f"Starting {QUEUE_COMPONENT_NAME}...")

    container_name = f"clp-{QUEUE_COMPONENT_NAME}-{instance_id}"
    if container_exists(container_name):
        logger.info(f"{QUEUE_COMPONENT_NAME} already running.")
        return

    queue_logs_dir = clp_config.logs_directory / QUEUE_COMPONENT_NAME
    validate_queue_config(clp_config, queue_logs_dir)

    log_filename = "rabbitmq.log"

    # Generate config file
    config_filename = f"{container_name}.conf"
    host_config_file_path = clp_config.logs_directory / config_filename
    with open(host_config_file_path, "w") as f:
        f.write(f"default_user = {clp_config.queue.username}\n")
        f.write(f"default_pass = {clp_config.queue.password}\n")
        f.write(f"log.file = {log_filename}\n")

    # Create directories
    queue_logs_dir.mkdir(exist_ok=True, parents=True)

    # Start container
    rabbitmq_logs_dir = pathlib.Path("/") / "var" / "log" / "rabbitmq"
    mounts = [
        DockerMount(
            DockerMountType.BIND,
            host_config_file_path,
            pathlib.Path("/") / "etc" / "rabbitmq" / "rabbitmq.conf",
            True,
        ),
        DockerMount(DockerMountType.BIND, queue_logs_dir, rabbitmq_logs_dir),
    ]
    rabbitmq_pid_file_path = pathlib.Path("/") / "tmp" / "rabbitmq.pid"
    # fmt: off
    cmd = [
        "docker", "run",
        "-d",
        "--rm",
        "--name", container_name,
        # Override RABBITMQ_LOGS since the image sets it to *only* log to stdout
        "-e", f"RABBITMQ_LOGS={rabbitmq_logs_dir / log_filename}",
        "-e", f"RABBITMQ_PID_FILE={rabbitmq_pid_file_path}",
        "-u", f"{os.getuid()}:{os.getgid()}",
    ]
    # fmt: on
    append_docker_port_settings_for_host_ips(
        clp_config.queue.host, clp_config.queue.port, 5672, cmd
    )
    for mount in mounts:
        cmd.append("--mount")
        cmd.append(str(mount))
    cmd.append("rabbitmq:3.9.8")
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    # Wait for queue to start up
    rabbitmq_cmd = ["rabbitmq-diagnostics", "check_running"]
    if not wait_for_container_cmd(container_name, rabbitmq_cmd, 60):
        raise EnvironmentError(f"{QUEUE_COMPONENT_NAME} did not initialize in time")

    logger.info(f"Started {QUEUE_COMPONENT_NAME}.")


def start_redis(instance_id: str, clp_config: CLPConfig, conf_dir: pathlib.Path):
    logger.info(f"Starting {REDIS_COMPONENT_NAME}...")

    container_name = f"clp-{REDIS_COMPONENT_NAME}-{instance_id}"
    if container_exists(container_name):
        logger.info(f"{REDIS_COMPONENT_NAME} already running.")
        return

    redis_logs_dir = clp_config.logs_directory / REDIS_COMPONENT_NAME
    redis_data_dir = clp_config.data_directory / REDIS_COMPONENT_NAME

    base_config_file_path = conf_dir / "redis" / "redis.conf"
    validate_redis_config(clp_config, redis_data_dir, redis_logs_dir, base_config_file_path)

    config_filename = f"{container_name}.conf"
    host_config_file_path = clp_config.logs_directory / config_filename
    with open(base_config_file_path, "r") as base, open(host_config_file_path, "w") as full:
        for line in base.readlines():
            full.write(line)
        full.write(f"requirepass {clp_config.redis.password}\n")

    redis_data_dir.mkdir(exist_ok=True, parents=True)
    redis_logs_dir.mkdir(exist_ok=True, parents=True)

    # Start container
    config_file_path = pathlib.Path("/") / "usr" / "local" / "etc" / "redis" / "redis.conf"
    mounts = [
        DockerMount(DockerMountType.BIND, host_config_file_path, config_file_path, True),
        DockerMount(
            DockerMountType.BIND, redis_logs_dir, pathlib.Path("/") / "var" / "log" / "redis"
        ),
        DockerMount(DockerMountType.BIND, redis_data_dir, pathlib.Path("/") / "data"),
    ]
    # fmt: off
    cmd = [
        "docker", "run",
        "-d",
        "--rm",
        "--name", container_name,
        "-u", f"{os.getuid()}:{os.getgid()}",
    ]
    # fmt: on
    for mount in mounts:
        cmd.append("--mount")
        cmd.append(str(mount))
    append_docker_port_settings_for_host_ips(
        clp_config.redis.host, clp_config.redis.port, 6379, cmd
    )
    cmd.append("redis:7.2.4")
    cmd.append("redis-server")
    cmd.append(str(config_file_path))
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {REDIS_COMPONENT_NAME}.")


def start_results_cache(instance_id: str, clp_config: CLPConfig, conf_dir: pathlib.Path):
    logger.info(f"Starting {RESULTS_CACHE_COMPONENT_NAME}...")

    container_name = f"clp-{RESULTS_CACHE_COMPONENT_NAME}-{instance_id}"
    if container_exists(container_name):
        logger.info(f"{RESULTS_CACHE_COMPONENT_NAME} already running.")
        return

    data_dir = clp_config.data_directory / RESULTS_CACHE_COMPONENT_NAME
    logs_dir = clp_config.logs_directory / RESULTS_CACHE_COMPONENT_NAME

    validate_results_cache_config(clp_config, data_dir, logs_dir)

    data_dir.mkdir(exist_ok=True, parents=True)
    logs_dir.mkdir(exist_ok=True, parents=True)

    mounts = [
        DockerMount(
            DockerMountType.BIND, conf_dir / "mongo", pathlib.Path("/") / "etc" / "mongo", True
        ),
        DockerMount(DockerMountType.BIND, data_dir, pathlib.Path("/") / "data" / "db"),
        DockerMount(DockerMountType.BIND, logs_dir, pathlib.Path("/") / "var" / "log" / "mongodb"),
    ]
    # fmt: off
    cmd = [
        "docker", "run",
        "-d",
        "--rm",
        "--name", container_name,
        "-u", f"{os.getuid()}:{os.getgid()}",
    ]
    # fmt: on
    for mount in mounts:
        cmd.append("--mount")
        cmd.append(str(mount))
    append_docker_port_settings_for_host_ips(
        clp_config.results_cache.host, clp_config.results_cache.port, 27017, cmd
    )
    cmd.append("mongo:7.0.1")
    cmd.append("--config")
    cmd.append(str(pathlib.Path("/") / "etc" / "mongo" / "mongod.conf"))
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {RESULTS_CACHE_COMPONENT_NAME}.")


def start_compression_scheduler(
    instance_id: str,
    clp_config: CLPConfig,
    container_clp_config: CLPConfig,
    mounts: CLPDockerMounts,
):
    module_name = "job_orchestration.scheduler.compress.compression_scheduler"
    generic_start_scheduler(
        COMPRESSION_SCHEDULER_COMPONENT_NAME,
        module_name,
        instance_id,
        clp_config,
        container_clp_config,
        mounts,
    )


def start_search_scheduler(
    instance_id: str,
    clp_config: CLPConfig,
    container_clp_config: CLPConfig,
    mounts: CLPDockerMounts,
):
    module_name = "job_orchestration.scheduler.search.search_scheduler"
    generic_start_scheduler(
        SEARCH_SCHEDULER_COMPONENT_NAME,
        module_name,
        instance_id,
        clp_config,
        container_clp_config,
        mounts,
    )


def generic_start_scheduler(
    component_name: str,
    module_name: str,
    instance_id: str,
    clp_config: CLPConfig,
    container_clp_config: CLPConfig,
    mounts: CLPDockerMounts,
):
    logger.info(f"Starting {component_name}...")

    container_name = f"clp-{component_name}-{instance_id}"
    if container_exists(container_name):
        logger.info(f"{component_name} already running.")
        return

    container_config_filename = f"{container_name}.yml"
    container_config_file_path = clp_config.logs_directory / container_config_filename
    with open(container_config_file_path, "w") as f:
        yaml.safe_dump(container_clp_config.dump_to_primitive_dict(), f)

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)
    container_logs_dir = container_clp_config.logs_directory / component_name

    clp_site_packages_dir = CONTAINER_CLP_HOME / "lib" / "python3" / "site-packages"
    # fmt: off
    container_start_cmd = [
        "docker", "run",
        "-di",
        "--network", "host",
        "-w", str(CONTAINER_CLP_HOME),
        "--rm",
        "--name", container_name,
        "-e", f"PYTHONPATH={clp_site_packages_dir}",
        "-e", (
            f"BROKER_URL=amqp://"
            f"{container_clp_config.queue.username}:{container_clp_config.queue.password}@"
            f"{container_clp_config.queue.host}:{container_clp_config.queue.port}"
        ),
        "-e", (
            f"RESULT_BACKEND=redis://default:{container_clp_config.redis.password}@"
            f"{container_clp_config.redis.host}:{container_clp_config.redis.port}/"
            f"{container_clp_config.redis.search_backend_database}"
        ),
        "-e", f"CLP_LOGS_DIR={container_logs_dir}",
        "-e", f"CLP_LOGGING_LEVEL={clp_config.search_scheduler.logging_level}",
        "-u", f"{os.getuid()}:{os.getgid()}",
        "--mount", str(mounts.clp_home),
    ]
    # fmt: on
    necessary_mounts = [
        mounts.logs_dir,
    ]
    if COMPRESSION_SCHEDULER_COMPONENT_NAME == component_name:
        necessary_mounts.append(mounts.input_logs_dir)
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append("--mount")
            container_start_cmd.append(str(mount))
    container_start_cmd.append(clp_config.execution_container)

    # fmt: off
    scheduler_cmd = [
        "python3", "-u",
        "-m", module_name,
        "--config", str(container_clp_config.logs_directory / container_config_filename),
    ]
    # fmt: on
    cmd = container_start_cmd + scheduler_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {component_name}.")


def start_compression_worker(
    instance_id: str,
    clp_config: CLPConfig,
    container_clp_config: CLPConfig,
    num_cpus: int,
    mounts: CLPDockerMounts,
):
    celery_method = "job_orchestration.executor.compress"
    celery_route = f"{QueueName.COMPRESSION}"
    generic_start_worker(
        COMPRESSION_WORKER_COMPONENT_NAME,
        instance_id,
        clp_config,
        clp_config.compression_worker,
        container_clp_config,
        celery_method,
        celery_route,
        clp_config.redis.compression_backend_database,
        num_cpus,
        mounts,
    )


def start_search_worker(
    instance_id: str,
    clp_config: CLPConfig,
    container_clp_config: CLPConfig,
    num_cpus: int,
    mounts: CLPDockerMounts,
):
    celery_method = "job_orchestration.executor.search"
    celery_route = f"{QueueName.SEARCH}"
    generic_start_worker(
        SEARCH_WORKER_COMPONENT_NAME,
        instance_id,
        clp_config,
        clp_config.search_worker,
        container_clp_config,
        celery_method,
        celery_route,
        clp_config.redis.search_backend_database,
        num_cpus,
        mounts,
    )


def generic_start_worker(
    component_name: str,
    instance_id: str,
    clp_config: CLPConfig,
    worker_config: BaseModel,
    container_clp_config: CLPConfig,
    celery_method: str,
    celery_route: str,
    redis_database: int,
    num_cpus: int,
    mounts: CLPDockerMounts,
):
    logger.info(f"Starting {component_name}...")

    container_name = f"clp-{component_name}-{instance_id}"
    if container_exists(container_name):
        logger.info(f"{component_name} already running.")
        return

    validate_worker_config(clp_config)

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)
    container_logs_dir = container_clp_config.logs_directory / component_name

    # Create necessary directories
    clp_config.archive_output.directory.mkdir(parents=True, exist_ok=True)

    clp_site_packages_dir = CONTAINER_CLP_HOME / "lib" / "python3" / "site-packages"
    # fmt: off
    container_start_cmd = [
        "docker", "run",
        "-di",
        "--network", "host",
        "-w", str(CONTAINER_CLP_HOME),
        "--rm",
        "--name", container_name,
        "-e", f"PYTHONPATH={clp_site_packages_dir}",
        "-e", (
            f"BROKER_URL=amqp://"
            f"{container_clp_config.queue.username}:{container_clp_config.queue.password}@"
            f"{container_clp_config.queue.host}:{container_clp_config.queue.port}"
        ),
        "-e", (
            f"RESULT_BACKEND=redis://default:{container_clp_config.redis.password}@"
            f"{container_clp_config.redis.host}:{container_clp_config.redis.port}/{redis_database}"
        ),
        "-e", f"CLP_HOME={CONTAINER_CLP_HOME}",
        "-e", f"CLP_DATA_DIR={container_clp_config.data_directory}",
        "-e", f"CLP_ARCHIVE_OUTPUT_DIR={container_clp_config.archive_output.directory}",
        "-e", f"CLP_LOGS_DIR={container_logs_dir}",
        "-e", f"CLP_LOGGING_LEVEL={worker_config.logging_level}",
        "-e", f"CLP_STORAGE_ENGINE={clp_config.package.storage_engine}",
        "-u", f"{os.getuid()}:{os.getgid()}",
        "--mount", str(mounts.clp_home),
    ]
    # fmt: on
    necessary_mounts = [
        mounts.data_dir,
        mounts.logs_dir,
        mounts.archives_output_dir,
        mounts.input_logs_dir,
    ]
    for mount in necessary_mounts:
        if mount:
            container_start_cmd.append("--mount")
            container_start_cmd.append(str(mount))
    container_start_cmd.append(clp_config.execution_container)

    worker_cmd = [
        "python3",
        str(clp_site_packages_dir / "bin" / "celery"),
        "-A",
        celery_method,
        "worker",
        "--concurrency",
        str(num_cpus),
        "--loglevel",
        "WARNING",
        "-f",
        str(container_logs_dir / "worker.log"),
        "-Q",
        celery_route,
        "-n",
        component_name,
    ]
    cmd = container_start_cmd + worker_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {component_name}.")


def update_meteor_settings(
    parent_key_prefix: str,
    settings: typing.Dict[str, typing.Any],
    updates: typing.Dict[str, typing.Any],
):
    """
    Recursively updates the given Meteor settings object with the values from `updates`.

    :param parent_key_prefix: The prefix for keys at this level in the settings dictionary.
    :param settings: The settings to update.
    :param updates: The updates.
    :raises ValueError: If a key in `updates` doesn't exist in `settings`.
    """
    for key, value in updates.items():
        if key not in settings:
            error_msg = f"{parent_key_prefix}{key} is not a valid configuration key for the webui."
            raise ValueError(error_msg)
        if isinstance(value, dict):
            update_meteor_settings(f"{parent_key_prefix}{key}.", settings[key], value)
        else:
            settings[key] = updates[key]


def start_webui(instance_id: str, clp_config: CLPConfig, mounts: CLPDockerMounts):
    logger.info(f"Starting {WEBUI_COMPONENT_NAME}...")

    container_name = f"clp-{WEBUI_COMPONENT_NAME}-{instance_id}"
    if container_exists(container_name):
        logger.info(f"{WEBUI_COMPONENT_NAME} already running.")
        return

    webui_logs_dir = clp_config.logs_directory / WEBUI_COMPONENT_NAME
    node_path = str(
        CONTAINER_CLP_HOME / "var" / "www" / "programs" / "server" / "npm" / "node_modules"
    )
    settings_json_path = get_clp_home() / "var" / "www" / "settings.json"

    validate_webui_config(clp_config, webui_logs_dir, settings_json_path)

    # Create directories
    webui_logs_dir.mkdir(exist_ok=True, parents=True)

    container_webui_logs_dir = pathlib.Path("/") / "var" / "log" / WEBUI_COMPONENT_NAME
    with open(settings_json_path, "r") as settings_json_file:
        meteor_settings = json.loads(settings_json_file.read())
    meteor_settings_updates = {
        "private": {
            "SqlDbHost": clp_config.database.host,
            "SqlDbPort": clp_config.database.port,
            "SqlDbName": clp_config.database.name,
            "SqlDbSearchJobsTableName": SEARCH_JOBS_TABLE_NAME,
            "SqlDbClpArchivesTableName": f"{CLP_METADATA_TABLE_PREFIX}archives",
            "SqlDbClpFilesTableName": f"{CLP_METADATA_TABLE_PREFIX}files",
        }
    }
    update_meteor_settings("", meteor_settings, meteor_settings_updates)

    # Start container
    # fmt: off
    container_cmd = [
        "docker", "run",
        "-d",
        "--network", "host",
        "--rm",
        "--name", container_name,
        "-e", f"NODE_PATH={node_path}",
        "-e", f"MONGO_URL={clp_config.results_cache.get_uri()}",
        "-e", f"PORT={clp_config.webui.port}",
        "-e", f"ROOT_URL=http://{clp_config.webui.host}",
        "-e", f"METEOR_SETTINGS={json.dumps(meteor_settings)}",
        "-e", f"CLP_DB_USER={clp_config.database.username}",
        "-e", f"CLP_DB_PASS={clp_config.database.password}",
        "-e", f"WEBUI_LOGS_DIR={container_webui_logs_dir}",
        "-e", f"WEBUI_LOGGING_LEVEL={clp_config.webui.logging_level}",
        "-u", f"{os.getuid()}:{os.getgid()}",
    ]
    # fmt: on
    necessary_mounts = [
        mounts.clp_home,
        DockerMount(DockerMountType.BIND, webui_logs_dir, container_webui_logs_dir),
    ]
    for mount in necessary_mounts:
        if mount:
            container_cmd.append("--mount")
            container_cmd.append(str(mount))
    container_cmd.append(clp_config.execution_container)

    node_cmd = [
        str(CONTAINER_CLP_HOME / "bin" / "node"),
        str(CONTAINER_CLP_HOME / "var" / "www" / "launcher.js"),
        str(CONTAINER_CLP_HOME / "var" / "www" / "main.js"),
    ]
    cmd = container_cmd + node_cmd
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

    logger.info(f"Started {WEBUI_COMPONENT_NAME}.")


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Starts CLP")
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )

    component_args_parser = args_parser.add_subparsers(dest="target")
    component_args_parser.add_parser(CONTROLLER_TARGET_NAME)
    component_args_parser.add_parser(DB_COMPONENT_NAME)
    component_args_parser.add_parser(QUEUE_COMPONENT_NAME)
    component_args_parser.add_parser(REDIS_COMPONENT_NAME)
    component_args_parser.add_parser(RESULTS_CACHE_COMPONENT_NAME)
    component_args_parser.add_parser(COMPRESSION_SCHEDULER_COMPONENT_NAME)
    component_args_parser.add_parser(SEARCH_SCHEDULER_COMPONENT_NAME)
    component_args_parser.add_parser(COMPRESSION_WORKER_COMPONENT_NAME)
    component_args_parser.add_parser(SEARCH_WORKER_COMPONENT_NAME)
    component_args_parser.add_parser(WEBUI_COMPONENT_NAME)

    args_parser.add_argument(
        "--num-cpus",
        type=int,
        default=0,
        help="Number of logical CPU cores to use for compression and search",
    )

    parsed_args = args_parser.parse_args(argv[1:])

    if parsed_args.target:
        target = parsed_args.target
    else:
        target = ALL_TARGET_NAME

    try:
        check_dependencies()
    except:
        logger.exception("Dependency checking failed.")
        return -1

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = validate_and_load_config_file(
            config_file_path, default_config_file_path, clp_home
        )

        # Validate and load necessary credentials
        if target in (
            ALL_TARGET_NAME,
            CONTROLLER_TARGET_NAME,
            DB_COMPONENT_NAME,
            COMPRESSION_SCHEDULER_COMPONENT_NAME,
            SEARCH_SCHEDULER_COMPONENT_NAME,
            WEBUI_COMPONENT_NAME,
        ):
            validate_and_load_db_credentials_file(clp_config, clp_home, True)
        if target in (
            ALL_TARGET_NAME,
            CONTROLLER_TARGET_NAME,
            QUEUE_COMPONENT_NAME,
            COMPRESSION_SCHEDULER_COMPONENT_NAME,
            SEARCH_SCHEDULER_COMPONENT_NAME,
            COMPRESSION_WORKER_COMPONENT_NAME,
            SEARCH_WORKER_COMPONENT_NAME,
        ):
            validate_and_load_queue_credentials_file(clp_config, clp_home, True)
        if target in (
            ALL_TARGET_NAME,
            CONTROLLER_TARGET_NAME,
            REDIS_COMPONENT_NAME,
            COMPRESSION_SCHEDULER_COMPONENT_NAME,
            SEARCH_SCHEDULER_COMPONENT_NAME,
            COMPRESSION_WORKER_COMPONENT_NAME,
            SEARCH_WORKER_COMPONENT_NAME,
        ):
            validate_and_load_redis_credentials_file(clp_config, clp_home, True)

        clp_config.validate_data_dir()
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    # Get the number of CPU cores to use
    num_cpus = multiprocessing.cpu_count() // 2
    if (
        target in (COMPRESSION_WORKER_COMPONENT_NAME, SEARCH_WORKER_COMPONENT_NAME)
        and parsed_args.num_cpus != 0
    ):
        num_cpus = parsed_args.num_cpus

    container_clp_config, mounts = generate_container_config(clp_config, clp_home)

    # Create necessary directories
    clp_config.data_directory.mkdir(parents=True, exist_ok=True)
    clp_config.logs_directory.mkdir(parents=True, exist_ok=True)

    try:
        # Create instance-id file
        instance_id_file_path = clp_config.logs_directory / "instance-id"
        if instance_id_file_path.exists():
            with open(instance_id_file_path, "r") as f:
                instance_id = f.readline()
        else:
            instance_id = str(uuid.uuid4())[-4:]
            with open(instance_id_file_path, "w") as f:
                f.write(instance_id)
                f.flush()

        conf_dir = clp_home / "etc"

        # Start components
        if target in (ALL_TARGET_NAME, DB_COMPONENT_NAME):
            start_db(instance_id, clp_config, conf_dir)
        if target in (ALL_TARGET_NAME, CONTROLLER_TARGET_NAME, DB_COMPONENT_NAME):
            create_db_tables(instance_id, clp_config, container_clp_config, mounts)
        if target in (ALL_TARGET_NAME, CONTROLLER_TARGET_NAME, QUEUE_COMPONENT_NAME):
            start_queue(instance_id, clp_config)
        if target in (ALL_TARGET_NAME, CONTROLLER_TARGET_NAME, REDIS_COMPONENT_NAME):
            start_redis(instance_id, clp_config, conf_dir)
        if target in (ALL_TARGET_NAME, RESULTS_CACHE_COMPONENT_NAME):
            start_results_cache(instance_id, clp_config, conf_dir)
        if target in (
            ALL_TARGET_NAME,
            CONTROLLER_TARGET_NAME,
            COMPRESSION_SCHEDULER_COMPONENT_NAME,
        ):
            start_compression_scheduler(instance_id, clp_config, container_clp_config, mounts)
        if target in (ALL_TARGET_NAME, CONTROLLER_TARGET_NAME, SEARCH_SCHEDULER_COMPONENT_NAME):
            start_search_scheduler(instance_id, clp_config, container_clp_config, mounts)
        if target in (ALL_TARGET_NAME, COMPRESSION_WORKER_COMPONENT_NAME):
            start_compression_worker(
                instance_id, clp_config, container_clp_config, num_cpus, mounts
            )
        if target in (ALL_TARGET_NAME, SEARCH_WORKER_COMPONENT_NAME):
            start_search_worker(instance_id, clp_config, container_clp_config, num_cpus, mounts)
        if target in (ALL_TARGET_NAME, CONTROLLER_TARGET_NAME, WEBUI_COMPONENT_NAME):
            start_webui(instance_id, clp_config, mounts)

    except Exception as ex:
        # Stop CLP
        subprocess.run([str(clp_home / "sbin" / "stop-clp.sh")], check=True)

        if type(ex) == ValueError:
            logger.error(f"Failed to start CLP: {ex}")
        else:
            logger.exception("Failed to start CLP.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
