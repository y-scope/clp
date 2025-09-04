import json
import logging
import multiprocessing
import os
import pathlib
import socket
import stat
import subprocess
from abc import ABC, abstractmethod
from typing import Any, Dict

from clp_py_utils.clp_config import (
    AwsAuthType,
    CLPConfig,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    DB_COMPONENT_NAME,
    GARBAGE_COLLECTOR_COMPONENT_NAME,
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
    check_docker_dependencies,
    CONTAINER_CLP_HOME,
    generate_docker_compose_container_config,
    get_clp_home,
    validate_db_config,
    validate_queue_config,
    validate_redis_config,
    validate_results_cache_config,
    validate_webui_config,
)

LOGS_FILE_MODE = stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH

logger = logging.getLogger(__name__)


def _get_ip_from_hostname(hostname: str) -> str:
    """
    Resolves a hostname to an IP address.

    :param hostname: The hostname to resolve.
    :return: The resolved IP address.
    """
    return socket.gethostbyname(hostname)


class BaseController(ABC):
    def __init__(self, clp_config: CLPConfig):
        self.clp_config = clp_config
        self.clp_home = get_clp_home()
        self._conf_dir = self.clp_home / "etc"

    def provision_database(self):
        component_name = DB_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        conf_file = self._conf_dir / "mysql" / "conf.d" / "logging.cnf"
        data_dir = self.clp_config.data_directory / component_name
        logs_dir = self.clp_config.logs_directory / component_name
        validate_db_config(self.clp_config, conf_file, data_dir, logs_dir)
        data_dir.mkdir(exist_ok=True, parents=True)
        logs_dir.mkdir(exist_ok=True, parents=True)

        return {
            "CLP_DB_CONF_FILE_HOST": str(conf_file),
            "CLP_DB_DATA_DIR_HOST": str(data_dir),
            "CLP_DB_LOGS_DIR_HOST": str(logs_dir),
            "CLP_DB_HOST": _get_ip_from_hostname(self.clp_config.database.host),
            "CLP_DB_PORT": str(self.clp_config.database.port),
            "CLP_DB_NAME": self.clp_config.database.name,
            "CLP_DB_USER": self.clp_config.database.username,
            "CLP_DB_PASS": self.clp_config.database.password,
            "CLP_DB_IMAGE": (
                "mysql:8.0.23" if "mysql" == self.clp_config.database.type else "mariadb:10-jammy"
            ),
        }

    def provision_queue(self):
        component_name = QUEUE_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        logs_dir = self.clp_config.logs_directory / component_name
        validate_queue_config(self.clp_config, logs_dir)
        logs_dir.mkdir(exist_ok=True, parents=True)

        return {
            "CLP_QUEUE_LOGS_DIR_HOST": str(logs_dir),
            "CLP_QUEUE_HOST": _get_ip_from_hostname(self.clp_config.queue.host),
            "CLP_QUEUE_PORT": str(self.clp_config.queue.port),
            "CLP_QUEUE_USER": self.clp_config.queue.username,
            "CLP_QUEUE_PASS": self.clp_config.queue.password,
        }

    def provision_redis(self):
        component_name = REDIS_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        conf_file = self._conf_dir / "redis" / "redis.conf"
        logs_dir = self.clp_config.logs_directory / component_name
        data_dir = self.clp_config.data_directory / component_name
        validate_redis_config(self.clp_config, conf_file, data_dir, logs_dir)
        data_dir.mkdir(exist_ok=True, parents=True)
        logs_dir.mkdir(exist_ok=True, parents=True)

        return {
            "CLP_REDIS_CONF_FILE_HOST": str(conf_file),
            "CLP_REDIS_DATA_DIR_HOST": str(data_dir),
            "CLP_REDIS_LOGS_DIR_HOST": str(logs_dir),
            "CLP_REDIS_HOST": _get_ip_from_hostname(self.clp_config.redis.host),
            "CLP_REDIS_PORT": str(self.clp_config.redis.port),
            "CLP_REDIS_PASS": self.clp_config.redis.password,
            "CLP_REDIS_QUERY_BACKEND_DB": str(self.clp_config.redis.query_backend_database),
            "CLP_REDIS_COMPRESSION_BACKEND_DB": str(
                self.clp_config.redis.compression_backend_database
            ),
        }

    def provision_results_cache(self):
        component_name = RESULTS_CACHE_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        conf_file = self._conf_dir / "mongo" / "mongod.conf"
        data_dir = self.clp_config.data_directory / component_name
        logs_dir = self.clp_config.logs_directory / component_name
        validate_results_cache_config(self.clp_config, conf_file, data_dir, logs_dir)
        data_dir.mkdir(exist_ok=True, parents=True)
        logs_dir.mkdir(exist_ok=True, parents=True)

        return {
            "CLP_RESULTS_CACHE_CONF_DIR_HOST": str(conf_file),
            "CLP_RESULTS_CACHE_DATA_DIR_HOST": str(data_dir),
            "CLP_RESULTS_CACHE_LOGS_DIR_HOST": str(logs_dir),
            "CLP_RESULTS_CACHE_HOST": _get_ip_from_hostname(self.clp_config.results_cache.host),
            "CLP_RESULTS_CACHE_PORT": str(self.clp_config.results_cache.port),
            "CLP_RESULTS_CACHE_DB_NAME": self.clp_config.results_cache.db_name,
            "CLP_RESULTS_CACHE_STREAM_COLLECTION_NAME": self.clp_config.results_cache.stream_collection_name,
        }

    def provision_compression_scheduler(self):
        component_name = COMPRESSION_SCHEDULER_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        logs_file = self.clp_config.logs_directory / f"{component_name}.log"
        logs_file.touch(mode=LOGS_FILE_MODE, exist_ok=True)

        return {
            "CLP_COMPRESSION_SCHEDULER_LOGGING_LEVEL": self.clp_config.compression_scheduler.logging_level,
            "CLP_COMPRESSION_SCHEDULER_LOGS_FILE_HOST": str(logs_file),
        }

    def provision_query_scheduler(self):
        component_name = QUERY_SCHEDULER_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        logs_file = self.clp_config.logs_directory / f"{component_name}.log"
        logs_file.touch(mode=LOGS_FILE_MODE, exist_ok=True)

        return {
            "CLP_QUERY_SCHEDULER_LOGGING_LEVEL": self.clp_config.query_scheduler.logging_level,
            "CLP_QUERY_SCHEDULER_LOGS_FILE_HOST": str(logs_file),
        }

    def provision_compression_worker(self, num_workers: int):
        component_name = COMPRESSION_WORKER_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        logs_dir = self.clp_config.logs_directory / component_name
        logs_dir.mkdir(parents=True, exist_ok=True)

        return {
            "CLP_COMPRESSION_WORKER_CONCURRENCY": str(num_workers),
            "CLP_COMPRESSION_WORKER_LOGGING_LEVEL": self.clp_config.compression_worker.logging_level,
            "CLP_COMPRESSION_WORKER_LOGS_DIR_HOST": str(logs_dir),
        }

    def provision_query_worker(self, num_workers: int):
        component_name = QUERY_WORKER_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        logs_dir = self.clp_config.logs_directory / component_name
        logs_dir.mkdir(parents=True, exist_ok=True)

        return {
            "CLP_QUERY_WORKER_LOGGING_LEVEL": self.clp_config.query_worker.logging_level,
            "CLP_QUERY_WORKER_LOGS_DIR": str(logs_dir),
            "CLP_QUERY_WORKER_CONCURRENCY": str(num_workers),
        }

    def provision_reducer(self, num_workers: int):
        component_name = REDUCER_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        logs_dir = self.clp_config.logs_directory / component_name
        logs_dir.mkdir(parents=True, exist_ok=True)

        return {
            "CLP_REDUCER_LOGGING_LEVEL": self.clp_config.reducer.logging_level,
            "CLP_REDUCER_LOGS_DIR": str(logs_dir),
            "CLP_REDUCER_CONCURRENCY": str(num_workers),
            "CLP_REDUCER_UPSERT_INTERVAL": str(self.clp_config.reducer.upsert_interval),
        }

    def _update_settings_object(
        self,
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
                error_msg = (
                    f"{parent_key_prefix}{key} is not a valid configuration key for the webui."
                )
                raise ValueError(error_msg)
            if isinstance(value, dict):
                self._update_settings_object(f"{parent_key_prefix}{key}.", settings[key], value)
            else:
                settings[key] = updates[key]

    def _read_and_update_settings_json(
        self, settings_file_path: pathlib.Path, updates: Dict[str, Any]
    ):
        """
        Reads and updates a settings JSON file.

        :param settings_file_path:
        :param updates:
        """
        with open(settings_file_path, "r") as settings_json_file:
            settings_object = json.loads(settings_json_file.read())
        self._update_settings_object("", settings_object, updates)

        return settings_object

    def provision_webui(self, container_clp_config: CLPConfig):
        component_name = WEBUI_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        container_webui_dir = CONTAINER_CLP_HOME / "var" / "www" / "webui"
        client_settings_json_path = (
            self.clp_home / "var" / "www" / "webui" / "client" / "settings.json"
        )
        server_settings_json_path = (
            self.clp_home / "var" / "www" / "webui" / "server" / "dist" / "settings.json"
        )

        validate_webui_config(self.clp_config, client_settings_json_path, server_settings_json_path)

        # Read, update, and write back client's and server's settings.json
        clp_db_connection_params = self.clp_config.database.get_clp_connection_params_and_type(True)
        table_prefix = clp_db_connection_params["table_prefix"]
        if StorageEngine.CLP_S == self.clp_config.package.storage_engine:
            archives_table_name = ""
            files_table_name = ""
        else:
            archives_table_name = get_archives_table_name(table_prefix, None)
            files_table_name = get_files_table_name(table_prefix, None)

        client_settings_json_updates = {
            "ClpStorageEngine": self.clp_config.package.storage_engine,
            "ClpQueryEngine": self.clp_config.package.query_engine,
            "MongoDbSearchResultsMetadataCollectionName": self.clp_config.webui.results_metadata_collection_name,
            "SqlDbClpArchivesTableName": archives_table_name,
            "SqlDbClpDatasetsTableName": get_datasets_table_name(table_prefix),
            "SqlDbClpFilesTableName": files_table_name,
            "SqlDbClpTablePrefix": table_prefix,
            "SqlDbCompressionJobsTableName": "compression_jobs",
        }
        client_settings_json = self._read_and_update_settings_json(
            client_settings_json_path, client_settings_json_updates
        )
        with open(client_settings_json_path, "w") as client_settings_json_file:
            client_settings_json_file.write(json.dumps(client_settings_json))

        server_settings_json_updates = {
            "SqlDbHost": container_clp_config.database.host,
            "SqlDbPort": self.clp_config.database.port,
            "SqlDbName": self.clp_config.database.name,
            "SqlDbQueryJobsTableName": "query_jobs",
            "MongoDbHost": container_clp_config.results_cache.host,
            "MongoDbPort": self.clp_config.results_cache.port,
            "MongoDbName": self.clp_config.results_cache.db_name,
            "MongoDbSearchResultsMetadataCollectionName": self.clp_config.webui.results_metadata_collection_name,
            "MongoDbStreamFilesCollectionName": self.clp_config.results_cache.stream_collection_name,
            "ClientDir": str(container_webui_dir / "client"),
            "LogViewerDir": str(container_webui_dir / "yscope-log-viewer"),
            "StreamTargetUncompressedSize": self.clp_config.stream_output.target_uncompressed_size,
        }

        stream_storage = self.clp_config.stream_output.storage
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

        server_settings_json = self._read_and_update_settings_json(
            server_settings_json_path, server_settings_json_updates
        )
        with open(server_settings_json_path, "w") as settings_json_file:
            settings_json_file.write(json.dumps(server_settings_json))

        return {
            "CLP_WEBUI_HOST": _get_ip_from_hostname(self.clp_config.webui.host),
            "CLP_WEBUI_PORT": str(self.clp_config.webui.port),
            "CLP_WEBUI_RATE_LIMIT": str(self.clp_config.webui.rate_limit),
        }

    def provision_garbage_collector(self):
        component_name = GARBAGE_COLLECTOR_COMPONENT_NAME
        logger.info(f"Provisioning {component_name}...")

        logs_dir = self.clp_config.logs_directory / component_name
        logs_dir.mkdir(parents=True, exist_ok=True)

        return {"CLP_GC_LOGGING_LEVEL": self.clp_config.garbage_collector.logging_level}

    @abstractmethod
    def deploy(self):
        """
        Deploys the provisioned components with orchestrator-specific logic.
        """
        pass

    @abstractmethod
    def stop(self):
        """
        Stops the deployed components with orchestrator-specific logic.
        """
        pass

    @abstractmethod
    def _provision(self) -> Dict[str, str]:
        """
        Provisions all components with orchestrator-specific logic.

        :return: Dictionary of environment variables for the orchestrator
        """
        pass


class DockerComposeController(BaseController):
    @staticmethod
    def _get_num_workers():
        """
        Gets the parallelism number for worker components.
        TODO: Revisit after moving from single-container to multi-container workers.
        """
        return multiprocessing.cpu_count() // 2

    def __init__(self, clp_config: CLPConfig):
        super().__init__(clp_config)

    def deploy(self):
        check_docker_dependencies(should_compose_run=False)
        self._provision()

        logger.info(f"Starting CLP using Docker Compose...")
        try:
            subprocess.run(
                ["docker", "compose", "up", "-d"],
                cwd=self.clp_home,
                stderr=subprocess.STDOUT,
                check=True,
            )
        except subprocess.CalledProcessError:
            logger.exception("Failed to start CLP.")
            raise

    def stop(self):
        check_docker_dependencies(should_compose_run=True)

        logger.info("Stopping all CLP containers using Docker Compose...")
        try:
            subprocess.run(
                ["docker", "compose", "down"],
                cwd=self.clp_home,
                stderr=subprocess.STDOUT,
                check=True,
            )
            logger.info("All CLP containers stopped.")
        except subprocess.CalledProcessError:
            logger.exception("Failed to stop CLP containers using Docker Compose.")
            raise

    def _provision(self):
        # Create necessary directories
        self.clp_config.data_directory.mkdir(parents=True, exist_ok=True)
        self.clp_config.logs_directory.mkdir(parents=True, exist_ok=True)
        self.clp_config.archive_output.get_directory().mkdir(parents=True, exist_ok=True)
        self.clp_config.stream_output.get_directory().mkdir(parents=True, exist_ok=True)

        container_clp_config = generate_docker_compose_container_config(self.clp_config)
        num_workers = self._get_num_workers()

        env_dict = {
            "CLP_PACKAGE_STORAGE_ENGINE": self.clp_config.package.storage_engine,
            # User and group IDs
            "CLP_USER_ID": str(os.getuid()),
            "CLP_GROUP_ID": str(os.getgid()),
            # Package container
            "CLP_PACKAGE_CONTAINER": self.clp_config.execution_container,
            # Global paths
            "CLP_DATA_DIR_HOST": str(self.clp_config.data_directory),
            "CLP_LOGS_DIR_HOST": str(self.clp_config.logs_directory),
            "CLP_ARCHIVE_OUTPUT_DIR_HOST": str(self.clp_config.archive_output.get_directory()),
            "CLP_STREAM_OUTPUT_DIR_HOST": str(self.clp_config.stream_output.get_directory()),
            # AWS credentials
            "CLP_AWS_ACCESS_KEY_ID": os.getenv("AWS_ACCESS_KEY_ID", ""),
            "CLP_AWS_SECRET_ACCESS_KEY": os.getenv("AWS_SECRET_ACCESS_KEY", ""),
            **self.provision_database(),
            **self.provision_queue(),
            **self.provision_redis(),
            **self.provision_results_cache(),
            **self.provision_compression_scheduler(),
            **self.provision_query_scheduler(),
            **self.provision_compression_worker(num_workers),
            **self.provision_query_worker(num_workers),
            **self.provision_reducer(num_workers),
            **self.provision_webui(container_clp_config),
            **self.provision_garbage_collector(),
        }

        if self.clp_config.aws_config_directory is not None:
            env_dict["CLP_AWS_CONFIG_DIR_HOST"] = str(self.clp_config.aws_config_directory)

        with open(f"{self.clp_home}/.env", "w") as env_file:
            for key, value in env_dict.items():
                env_file.write(f"{key}={value}\n")
