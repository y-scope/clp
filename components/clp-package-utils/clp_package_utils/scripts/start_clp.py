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
from clp_py_utils.clp_config import (
    CLPConfig,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    DB_COMPONENT_NAME,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    SEARCH_SCHEDULER_COMPONENT_NAME,
    SEARCH_WORKER_COMPONENT_NAME,
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


def wait_for_database_to_init(container_name: str, clp_config: CLPConfig, timeout: int):
    # Try to connect to the database
    begin_time = time.time()
    container_exec_cmd = ["docker", "exec", "-it", container_name]
    # fmt: off
    mysqladmin_cmd = [
        "mysqladmin", "ping",
        "--silent",
        "-h", "127.0.0.1",
        "-u", str(clp_config.database.username),
        f"--password={clp_config.database.password}",
    ]
    # fmt: on
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

    if not wait_for_database_to_init(container_name, clp_config, 30):
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
    # fmt: off
    cmd = [
        "docker", "exec", "-it", container_name,
        "rabbitmqctl", "wait", str(rabbitmq_pid_file_path),
    ]
    # fmt: on
    subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

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

    component_args_parser = args_parser.add_subparsers(dest="component_name")
    component_args_parser.add_parser(DB_COMPONENT_NAME)
    component_args_parser.add_parser(QUEUE_COMPONENT_NAME)
    component_args_parser.add_parser(REDIS_COMPONENT_NAME)
    component_args_parser.add_parser(RESULTS_CACHE_COMPONENT_NAME)
    component_args_parser.add_parser(COMPRESSION_SCHEDULER_COMPONENT_NAME)
    component_args_parser.add_parser(SEARCH_SCHEDULER_COMPONENT_NAME)
    component_args_parser.add_parser(COMPRESSION_WORKER_COMPONENT_NAME)
    component_args_parser.add_parser(SEARCH_WORKER_COMPONENT_NAME)
    args_parser.add_argument(
        "--num-cpus",
        type=int,
        default=0,
        help="Number of logical CPU cores to use for compression and search",
    )

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
        clp_config = validate_and_load_config_file(
            config_file_path, default_config_file_path, clp_home
        )

        # Validate and load necessary credentials
        if component_name in [
            "",
            DB_COMPONENT_NAME,
            COMPRESSION_SCHEDULER_COMPONENT_NAME,
            SEARCH_SCHEDULER_COMPONENT_NAME,
        ]:
            validate_and_load_db_credentials_file(clp_config, clp_home, True)
        if component_name in [
            "",
            QUEUE_COMPONENT_NAME,
            COMPRESSION_SCHEDULER_COMPONENT_NAME,
            SEARCH_SCHEDULER_COMPONENT_NAME,
            COMPRESSION_WORKER_COMPONENT_NAME,
            SEARCH_WORKER_COMPONENT_NAME,
        ]:
            validate_and_load_queue_credentials_file(clp_config, clp_home, True)
        if component_name in [
            "",
            REDIS_COMPONENT_NAME,
            COMPRESSION_SCHEDULER_COMPONENT_NAME,
            SEARCH_SCHEDULER_COMPONENT_NAME,
            COMPRESSION_WORKER_COMPONENT_NAME,
            SEARCH_WORKER_COMPONENT_NAME,
        ]:
            validate_and_load_redis_credentials_file(clp_config, clp_home, True)

        clp_config.validate_data_dir()
        clp_config.validate_logs_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    # Get the number of CPU cores to use
    num_cpus = multiprocessing.cpu_count() // 2
    if (
        COMPRESSION_WORKER_COMPONENT_NAME == component_name
        or SEARCH_WORKER_COMPONENT_NAME == component_name
    ) and parsed_args.num_cpus != 0:
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
        if "" == component_name or DB_COMPONENT_NAME == component_name:
            start_db(instance_id, clp_config, conf_dir)
            create_db_tables(instance_id, clp_config, container_clp_config, mounts)
        if "" == component_name or QUEUE_COMPONENT_NAME == component_name:
            start_queue(instance_id, clp_config)
        if "" == component_name or REDIS_COMPONENT_NAME == component_name:
            start_redis(instance_id, clp_config, conf_dir)
        if "" == component_name or RESULTS_CACHE_COMPONENT_NAME == component_name:
            start_results_cache(instance_id, clp_config, conf_dir)
        if "" == component_name or COMPRESSION_SCHEDULER_COMPONENT_NAME == component_name:
            start_compression_scheduler(instance_id, clp_config, container_clp_config, mounts)
        if "" == component_name or SEARCH_SCHEDULER_COMPONENT_NAME == component_name:
            start_search_scheduler(instance_id, clp_config, container_clp_config, mounts)
        if "" == component_name or COMPRESSION_WORKER_COMPONENT_NAME == component_name:
            start_compression_worker(
                instance_id, clp_config, container_clp_config, num_cpus, mounts
            )
        if "" == component_name or SEARCH_WORKER_COMPONENT_NAME == component_name:
            start_search_worker(instance_id, clp_config, container_clp_config, num_cpus, mounts)

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
