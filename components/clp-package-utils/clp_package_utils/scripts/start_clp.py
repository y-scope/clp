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
from typing import Any, Dict, List, Optional

from clp_py_utils.clp_config import (
    AwsAuthType,
    CLPConfig,
    COMPRESSION_JOBS_TABLE_NAME,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    CONTROLLER_TARGET_NAME,
    DB_COMPONENT_NAME,
    GARBAGE_COLLECTOR_COMPONENT_NAME,
    QUERY_JOBS_TABLE_NAME,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    StorageEngine,
    StorageType,
    WEBUI_COMPONENT_NAME,
)
from clp_py_utils.clp_metadata_db_utils import (
    get_archives_table_name,
    get_datasets_table_name,
    get_files_table_name,
)

from clp_package_utils.general import (
    check_dependencies,
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CONTAINER_CLP_HOME,
    DockerMount,
    dump_shared_container_config,
    generate_docker_compose_container_config,
    get_clp_home,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_and_load_queue_credentials_file,
    validate_and_load_redis_credentials_file,
    validate_db_config,
    validate_logs_input_config,
    validate_output_storage_config,
    validate_queue_config,
    validate_redis_config,
    validate_results_cache_config,
    validate_retention_config,
    validate_webui_config,
)

logger = logging.getLogger(__file__)
env_dict = {}


def get_ip_from_hostname(hostname: str) -> str:
    return socket.gethostbyname(hostname)


def append_docker_options(
    cmd: List[str],
    mounts: Optional[List[Optional[DockerMount]]] = None,
    env_vars: Optional[List[str]] = None,
):
    """
    Appends Docker mount and environment variable options to a command list.

    :param cmd: The command list to append options to.
    :param mounts: Optional list of DockerMount objects to add as --mount options.
    :param env_vars: Optional list of environment variables to add as -e options.
    """
    if mounts:
        for mount in mounts:
            if mount:
                cmd.append("--mount")
                cmd.append(str(mount))

    if env_vars:
        for env_var in env_vars:
            if "" != env_var:
                cmd.append("-e")
                cmd.append(env_var)


def append_docker_port_settings_for_host_ips(
    hostname: str, host_port: int, container_port: int, cmd: [str]
):
    # Note: We use a set because gethostbyname_ex can return the same IP twice for one hostname
    for ip in set(socket.gethostbyname_ex(hostname)[2]):
        cmd.append("-p")
        cmd.append(f"{ip}:{host_port}:{container_port}")


def chown_recursively(
    path: pathlib.Path,
    user_id: int,
    group_id: int,
):
    """
    Recursively changes the owner of the given path to the given user ID and group ID.
    :param path:
    :param user_id:
    :param group_id:
    """
    chown_cmd = ["chown", "--recursive", f"{user_id}:{group_id}", str(path)]
    subprocess.run(chown_cmd, stdout=subprocess.DEVNULL, check=True)


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


def start_db(clp_config: CLPConfig, conf_dir: pathlib.Path):
    component_name = DB_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    db_data_dir = clp_config.data_directory / component_name
    db_logs_dir = clp_config.logs_directory / component_name

    validate_db_config(clp_config, db_data_dir, db_logs_dir)

    # Create directories
    db_data_dir.mkdir(exist_ok=True, parents=True)
    db_logs_dir.mkdir(exist_ok=True, parents=True)

    # Start container
    env_dict["CLP_HOST_DB_CONF_DIR"] = str(conf_dir / "mysql" / "conf.d")
    env_dict["CLP_HOST_DB_DATA_DIR"] = str(db_data_dir)
    env_dict["CLP_HOST_DB_LOGS_DIR"] = str(db_logs_dir)

    # TODO: Consider a Docker Compose overwrite / extend approach for pick the right image.
    if "mysql" == clp_config.database.type:
        env_dict["CLP_DB_IMAGE"] = "mysql:8.0.23"
    elif "mariadb" == clp_config.database.type:
        env_dict["CLP_DB_IMAGE"] = "mariadb:10-jammy"

    env_dict["CLP_DB_HOST"] = get_ip_from_hostname(clp_config.database.host)
    env_dict["CLP_DB_PORT"] = str(clp_config.database.port)
    env_dict["CLP_DB_NAME"] = clp_config.database.name
    env_dict["CLP_DB_USER"] = clp_config.database.username
    env_dict["CLP_DB_PASS"] = clp_config.database.password


def start_queue(clp_config: CLPConfig):
    component_name = QUEUE_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    queue_logs_dir = clp_config.logs_directory / component_name
    validate_queue_config(clp_config, queue_logs_dir)

    env_dict["CLP_QUEUE_HOST"] = get_ip_from_hostname(clp_config.queue.host)
    env_dict["CLP_QUEUE_PORT"] = str(clp_config.queue.port)
    env_dict["CLP_QUEUE_USER"] = clp_config.queue.username
    env_dict["CLP_QUEUE_PASS"] = clp_config.queue.password

    # Create directories
    queue_logs_dir.mkdir(exist_ok=True, parents=True)

    env_dict["CLP_HOST_QUEUE_LOGS_DIR"] = str(queue_logs_dir)


def start_redis(clp_config: CLPConfig, conf_dir: pathlib.Path):
    component_name = REDIS_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    redis_logs_dir = clp_config.logs_directory / component_name
    redis_data_dir = clp_config.data_directory / component_name

    config_file_path = conf_dir / "redis" / "redis.conf"
    validate_redis_config(clp_config, redis_data_dir, redis_logs_dir, config_file_path)

    env_dict["CLP_HOST_REDIS_CONF_PATH"] = str(config_file_path)
    env_dict["CLP_HOST_REDIS_DATA_DIR"] = str(redis_data_dir)
    env_dict["CLP_HOST_REDIS_LOGS_DIR"] = str(redis_logs_dir)

    redis_data_dir.mkdir(exist_ok=True, parents=True)
    redis_logs_dir.mkdir(exist_ok=True, parents=True)

    env_dict["CLP_REDIS_HOST"] = get_ip_from_hostname(clp_config.redis.host)
    env_dict["CLP_REDIS_PORT"] = str(clp_config.redis.port)
    env_dict["CLP_REDIS_PASS"] = clp_config.redis.password
    env_dict["CLP_REDIS_QUERY_BACKEND_DB"] = str(clp_config.redis.query_backend_database)
    env_dict["CLP_REDIS_COMPRESSION_BACKEND_DB"] = str(
        clp_config.redis.compression_backend_database
    )


def start_results_cache(clp_config: CLPConfig, conf_dir: pathlib.Path):
    component_name = RESULTS_CACHE_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    data_dir = clp_config.data_directory / component_name
    logs_dir = clp_config.logs_directory / component_name

    validate_results_cache_config(clp_config, data_dir, logs_dir)

    data_dir.mkdir(exist_ok=True, parents=True)
    logs_dir.mkdir(exist_ok=True, parents=True)

    env_dict["CLP_HOST_RESULTS_CACHE_CONF_DIR"] = str(conf_dir / "mongo")
    env_dict["CLP_HOST_RESULTS_CACHE_DATA_DIR"] = str(data_dir)
    env_dict["CLP_HOST_RESULTS_CACHE_LOGS_DIR"] = str(logs_dir)

    env_dict["CLP_RESULTS_CACHE_HOST"] = get_ip_from_hostname(clp_config.results_cache.host)
    env_dict["CLP_RESULTS_CACHE_PORT"] = str(clp_config.results_cache.port)
    env_dict["CLP_RESULTS_CACHE_DB_NAME"] = clp_config.results_cache.db_name
    env_dict["CLP_RESULTS_CACHE_STREAM_COLLECTION_NAME"] = (
        clp_config.results_cache.stream_collection_name
    )


def start_compression_scheduler(
    clp_config: CLPConfig,
):
    component_name = COMPRESSION_SCHEDULER_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)

    env_dict["CLP_HOST_COMPRESSION_SCHEDULER_LOGS_DIR"] = str(logs_dir)
    env_dict["CLP_COMPRESSION_SCHEDULER_LOGGING_LEVEL"] = (
        clp_config.compression_scheduler.logging_level
    )


def start_query_scheduler(
    clp_config: CLPConfig,
):
    component_name = QUERY_SCHEDULER_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)

    env_dict["CLP_HOST_QUERY_SCHEDULER_LOGS_DIR"] = str(logs_dir)
    env_dict["CLP_QUERY_SCHEDULER_LOGGING_LEVEL"] = clp_config.query_scheduler.logging_level


def start_compression_worker(
    clp_config: CLPConfig,
    num_cpus: int,
):
    component_name = COMPRESSION_WORKER_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)

    env_dict["CLP_COMPRESSION_WORKER_LOGS_DIR"] = str(logs_dir)
    env_dict["CLP_COMPRESSION_WORKER_LOGGING_LEVEL"] = clp_config.compression_worker.logging_level
    env_dict["CLP_COMPRESSION_WORKER_CONCURRENCY"] = str(num_cpus)

    # Create necessary directories
    clp_config.archive_output.get_directory().mkdir(parents=True, exist_ok=True)
    clp_config.stream_output.get_directory().mkdir(parents=True, exist_ok=True)


def start_query_worker(
    clp_config: CLPConfig,
    num_cpus: int,
):
    component_name = QUERY_WORKER_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)

    env_dict["CLP_QUERY_WORKER_LOGS_DIR"] = str(logs_dir)
    env_dict["CLP_QUERY_WORKER_LOGGING_LEVEL"] = clp_config.query_worker.logging_level
    env_dict["CLP_QUERY_WORKER_CONCURRENCY"] = str(num_cpus)

    # Create necessary directories
    clp_config.archive_output.get_directory().mkdir(parents=True, exist_ok=True)
    clp_config.stream_output.get_directory().mkdir(parents=True, exist_ok=True)


def start_reducer(
    clp_config: CLPConfig,
    num_workers: int,
):
    component_name = REDUCER_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)

    env_dict["CLP_REDUCER_LOGS_DIR"] = str(logs_dir)
    env_dict["CLP_REDUCER_LOGGING_LEVEL"] = clp_config.reducer.logging_level
    env_dict["CLP_REDUCER_CONCURRENCY"] = str(num_workers)
    env_dict["CLP_REDUCER_UPSERT_INTERVAL"] = str(clp_config.reducer.upsert_interval)


def update_settings_object(
    parent_key_prefix: str,
    settings: Dict[str, Any],
    updates: Dict[str, Any],
):
    """
    Recursively updates the given settings object with the values from `updates`.

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
            update_settings_object(f"{parent_key_prefix}{key}.", settings[key], value)
        else:
            settings[key] = updates[key]


def read_and_update_settings_json(settings_file_path: pathlib.Path, updates: Dict[str, Any]):
    """
    Reads and updates a settings JSON file.

    :param settings_file_path:
    :param updates:
    """
    with open(settings_file_path, "r") as settings_json_file:
        settings_object = json.loads(settings_json_file.read())
    update_settings_object("", settings_object, updates)

    return settings_object


def start_webui(
    clp_config: CLPConfig,
    container_clp_config: CLPConfig,
):
    component_name = WEBUI_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    container_webui_dir = CONTAINER_CLP_HOME / "var" / "www" / "webui"
    client_settings_json_path = (
        get_clp_home() / "var" / "www" / "webui" / "client" / "settings.json"
    )
    server_settings_json_path = (
        get_clp_home() / "var" / "www" / "webui" / "server" / "dist" / "server" / "settings.json"
    )

    validate_webui_config(clp_config, client_settings_json_path, server_settings_json_path)

    # Read, update, and write back client's and server's settings.json
    clp_db_connection_params = clp_config.database.get_clp_connection_params_and_type(True)
    table_prefix = clp_db_connection_params["table_prefix"]
    if StorageEngine.CLP_S == clp_config.package.storage_engine:
        archives_table_name = ""
        files_table_name = ""
    else:
        archives_table_name = get_archives_table_name(table_prefix, None)
        files_table_name = get_files_table_name(table_prefix, None)

    client_settings_json_updates = {
        "ClpStorageEngine": clp_config.package.storage_engine,
        "ClpQueryEngine": clp_config.package.query_engine,
        "MongoDbSearchResultsMetadataCollectionName": clp_config.webui.results_metadata_collection_name,
        "SqlDbClpArchivesTableName": archives_table_name,
        "SqlDbClpDatasetsTableName": get_datasets_table_name(table_prefix),
        "SqlDbClpFilesTableName": files_table_name,
        "SqlDbClpTablePrefix": table_prefix,
        "SqlDbCompressionJobsTableName": COMPRESSION_JOBS_TABLE_NAME,
    }
    client_settings_json = read_and_update_settings_json(
        client_settings_json_path, client_settings_json_updates
    )
    with open(client_settings_json_path, "w") as client_settings_json_file:
        client_settings_json_file.write(json.dumps(client_settings_json))

    server_settings_json_updates = {
        "SqlDbHost": container_clp_config.database.host,
        "SqlDbPort": clp_config.database.port,
        "SqlDbName": clp_config.database.name,
        "SqlDbQueryJobsTableName": QUERY_JOBS_TABLE_NAME,
        "MongoDbHost": container_clp_config.results_cache.host,
        "MongoDbPort": clp_config.results_cache.port,
        "MongoDbName": clp_config.results_cache.db_name,
        "MongoDbSearchResultsMetadataCollectionName": clp_config.webui.results_metadata_collection_name,
        "MongoDbStreamFilesCollectionName": clp_config.results_cache.stream_collection_name,
        "ClientDir": str(container_webui_dir / "client"),
        "LogViewerDir": str(container_webui_dir / "yscope-log-viewer"),
        "StreamTargetUncompressedSize": clp_config.stream_output.target_uncompressed_size,
    }

    stream_storage = clp_config.stream_output.storage
    if StorageType.S3 == stream_storage.type:
        s3_config = stream_storage.s3_config
        server_settings_json_updates["StreamFilesDir"] = None
        server_settings_json_updates["StreamFilesS3Region"] = s3_config.region_code
        server_settings_json_updates["StreamFilesS3PathPrefix"] = (
            f"{s3_config.bucket}/{s3_config.key_prefix}"
        )
        auth = s3_config.aws_authentication
        if AwsAuthType.profile == auth.type:
            server_settings_json_updates["StreamFilesS3Profile"] = auth.profile
        else:
            server_settings_json_updates["StreamFilesS3Profile"] = None
    elif StorageType.FS == stream_storage.type:
        server_settings_json_updates["StreamFilesDir"] = str(
            container_clp_config.stream_output.get_directory()
        )
        server_settings_json_updates["StreamFilesS3Region"] = None
        server_settings_json_updates["StreamFilesS3PathPrefix"] = None
        server_settings_json_updates["StreamFilesS3Profile"] = None

    server_settings_json = read_and_update_settings_json(
        server_settings_json_path, server_settings_json_updates
    )
    with open(server_settings_json_path, "w") as settings_json_file:
        settings_json_file.write(json.dumps(server_settings_json))

    env_dict["CLP_WEBUI_HOST"] = get_ip_from_hostname(clp_config.webui.host)
    env_dict["CLP_WEBUI_PORT"] = clp_config.webui.port


def start_garbage_collector(
    clp_config: CLPConfig,
):
    component_name = GARBAGE_COLLECTOR_COMPONENT_NAME
    logger.info(f"Initializing {component_name}...")

    logs_dir = clp_config.logs_directory / component_name
    logs_dir.mkdir(parents=True, exist_ok=True)

    env_dict["CLP_GC_LOGGING_LEVEL"] = clp_config.garbage_collector.logging_level


def add_num_workers_argument(parser):
    parser.add_argument(
        "--num-workers",
        type=int,
        default=multiprocessing.cpu_count(),
        help="Number of workers to start",
    )


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

    parsed_args = args_parser.parse_args(argv[1:])

    try:
        check_dependencies(should_compose_run=False)
    except:
        logger.exception("Dependency checking failed.")
        return -1

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)

        validate_and_load_db_credentials_file(clp_config, clp_home, True)
        validate_and_load_queue_credentials_file(clp_config, clp_home, True)
        validate_and_load_redis_credentials_file(clp_config, clp_home, True)
        validate_logs_input_config(clp_config)
        validate_output_storage_config(clp_config)
        validate_retention_config(clp_config)

        clp_config.validate_data_dir()
        clp_config.validate_logs_dir()
        clp_config.validate_aws_config_dir()
    except:
        logger.exception("Failed to load config.")
        return -1

    # TODO: Rely on Docker Compose to spawn multiple workers
    num_workers = multiprocessing.cpu_count() // 2

    container_clp_config = generate_docker_compose_container_config(clp_config)

    # Create necessary directories
    clp_config.data_directory.mkdir(parents=True, exist_ok=True)
    clp_config.logs_directory.mkdir(parents=True, exist_ok=True)

    dump_shared_container_config(container_clp_config, clp_config)

    env_dict["CLP_PACKAGE_CONTAINER"] = "clp-package-x86-ubuntu-jammy:dev"

    env_dict["CLP_USER_ID"] = os.getuid()
    env_dict["CLP_GROUP_ID"] = os.getgid()
    env_dict["CLP_STORAGE_ENGINE"] = clp_config.package.storage_engine

    env_dict["CLP_HOST_DATA_DIR"] = str(clp_config.data_directory)
    env_dict["CLP_HOST_LOGS_DIR"] = str(clp_config.logs_directory)
    env_dict["CLP_HOST_ARCHIVE_OUTPUT_DIR"] = str(clp_config.archive_output.get_directory())
    env_dict["CLP_HOST_STREAM_OUTPUT_DIR"] = str(clp_config.stream_output.get_directory())

    if clp_config.aws_config_directory is not None:
        env_dict["CLP_HOST_AWS_CONFIG_DIR"] = str(clp_config.aws_config_directory)
    if StorageType.S3 == clp_config.archive_output.storage.type:
        clp_config.archive_output.storage.staging_directory.mkdir(parents=True, exist_ok=True)
        env_dict["CLP_HOST_ARCHIVE_STAGING_DIR"] = str(
            clp_config.archive_output.storage.staging_directory
        )
    if StorageType.S3 == clp_config.stream_output.storage.type:
        clp_config.stream_output.storage.staging_directory.mkdir(parents=True, exist_ok=True)
        env_dict["CLP_HOST_STREAM_STAGING_DIR"] = str(
            clp_config.stream_output.storage.staging_directory
        )

    try:
        conf_dir = clp_home / "etc"

        # Start components
        start_db(clp_config, conf_dir)
        start_queue(clp_config)
        start_redis(clp_config, conf_dir)
        start_results_cache(clp_config, conf_dir)
        start_compression_scheduler(clp_config)
        start_query_scheduler(clp_config)
        start_compression_worker(clp_config, num_workers)
        start_query_worker(clp_config, num_workers)
        start_reducer(clp_config, num_workers)
        start_webui(clp_config, container_clp_config)
        start_garbage_collector(clp_config)

        with open(f"{clp_home}/.env", "w") as env_file:
            for key, value in env_dict.items():
                env_file.write(f"{key}={value}\n")
    except Exception as ex:
        if type(ex) == ValueError:
            logger.error(f"Failed to initialize CLP: {ex}")
        else:
            logger.exception("Failed to initialize CLP.")
        return -1

    logger.info(f"Starting CLP using Docker Compose...")
    try:
        subprocess.run(
            ["docker", "compose", "up", "-d"],
            stderr=subprocess.STDOUT,
            check=True,
        )
    except subprocess.CalledProcessError:
        logger.exception("Failed to start CLP.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
