import json
import logging
import multiprocessing
import os
import pathlib
import socket
import stat
import subprocess
import uuid
from abc import ABC, abstractmethod
from typing import Any, Optional

from clp_py_utils.clp_config import (
    AwsAuthType,
    CLPConfig,
    COMPRESSION_JOBS_TABLE_NAME,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    DB_COMPONENT_NAME,
    DeploymentType,
    GARBAGE_COLLECTOR_COMPONENT_NAME,
    OrchestrationType,
    QUERY_JOBS_TABLE_NAME,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    QueryEngine,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    SPIDER_SCHEDULER_COMPONENT_NAME,
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
    DockerComposeProjectNotRunningError,
    DockerDependencyError,
    dump_shared_container_config,
    generate_docker_compose_container_config,
    get_clp_home,
    is_retention_period_configured,
    validate_db_config,
    validate_queue_config,
    validate_redis_config,
    validate_results_cache_config,
    validate_webui_config,
)

LOG_FILE_ACCESS_MODE = stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH

DEFAULT_UID_GID = f"{os.getuid()}:{os.getgid()}"
THIRD_PARTY_SERVICE_UID = 999
THIRD_PARTY_SERVICE_GID = 999
THIRD_PARTY_SERVICE_UID_GID = f"{THIRD_PARTY_SERVICE_UID}:{THIRD_PARTY_SERVICE_GID}"

logger = logging.getLogger(__name__)


class EnvVarsDict(dict[str, Optional[str]]):
    def __ior__(self, other: "EnvVarsDict") -> "EnvVarsDict":
        """
        Overloads the `|=` operator for static type checking on `other`.
        """
        super().__ior__(other)
        return self


class BaseController(ABC):
    """
    Base controller for orchestrating CLP components. Derived classes should implement any
    orchestrator-specific logic. This class provides common logic for preparing environment
    variables, directories, and configuration files for each component.
    """

    def __init__(self, clp_config: CLPConfig) -> None:
        self._clp_config = clp_config
        self._clp_home = get_clp_home()
        self._conf_dir = self._clp_home / "etc"

    @abstractmethod
    def start(self) -> None:
        """
        Starts the components.
        """

    @abstractmethod
    def stop(self) -> None:
        """
        Stops the components.
        """

    @abstractmethod
    def _set_up_env(self) -> None:
        """
        Sets up all components to run by preparing environment variables, directories, and
        configuration files.
        """

    def _set_up_env_for_database(self) -> EnvVarsDict:
        """
        Sets up environment variables and directories for the database component.

        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = DB_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        conf_logging_file = self._conf_dir / "mysql" / "conf.d" / "logging.cnf"
        data_dir = self._clp_config.data_directory / component_name
        logs_dir = self._clp_config.logs_directory / component_name
        validate_db_config(self._clp_config, conf_logging_file, data_dir, logs_dir)

        data_dir.mkdir(exist_ok=True, parents=True)
        logs_dir.mkdir(exist_ok=True, parents=True)
        _chown_paths_if_root(data_dir, logs_dir)

        env_vars = EnvVarsDict()

        # Connection config
        env_vars |= {
            "CLP_DB_HOST": _get_ip_from_hostname(self._clp_config.database.host),
            "CLP_DB_NAME": self._clp_config.database.name,
            "CLP_DB_PORT": str(self._clp_config.database.port),
        }

        # Credentials
        env_vars |= {
            "CLP_DB_PASS": self._clp_config.database.password,
            "CLP_DB_USER": self._clp_config.database.username,
        }

        # Paths
        env_vars |= {
            "CLP_DB_CONF_LOGGING_FILE_HOST": str(conf_logging_file),
            "CLP_DB_DATA_DIR_HOST": str(data_dir),
            "CLP_DB_LOGS_DIR_HOST": str(logs_dir),
        }

        # Runtime config
        env_vars |= {
            "CLP_DB_CONTAINER_IMAGE_REF": (
                "mysql:8.0.23" if self._clp_config.database.type == "mysql" else "mariadb:10-jammy"
            ),
        }

        return env_vars

    def _set_up_env_for_queue(self) -> EnvVarsDict:
        """
        Sets up environment variables and directories for the message queue component.

        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = QUEUE_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        logs_dir = self._clp_config.logs_directory / component_name
        validate_queue_config(self._clp_config, logs_dir)

        logs_dir.mkdir(exist_ok=True, parents=True)
        _chown_paths_if_root(logs_dir)

        env_vars = EnvVarsDict()

        # Connection config
        env_vars |= {
            "CLP_QUEUE_HOST": _get_ip_from_hostname(self._clp_config.queue.host),
            "CLP_QUEUE_PORT": str(self._clp_config.queue.port),
        }

        # Credentials
        env_vars |= {
            "CLP_QUEUE_PASS": self._clp_config.queue.password,
            "CLP_QUEUE_USER": self._clp_config.queue.username,
        }

        # Paths
        env_vars |= {
            "CLP_QUEUE_LOGS_DIR_HOST": str(logs_dir),
        }

        return env_vars

    def _set_up_env_for_redis(self) -> EnvVarsDict:
        """
        Sets up environment variables and directories for the Redis component.

        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = REDIS_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        conf_file = self._conf_dir / "redis" / "redis.conf"
        data_dir = self._clp_config.data_directory / component_name
        logs_dir = self._clp_config.logs_directory / component_name
        validate_redis_config(self._clp_config, conf_file, data_dir, logs_dir)

        data_dir.mkdir(exist_ok=True, parents=True)
        logs_dir.mkdir(exist_ok=True, parents=True)
        _chown_paths_if_root(data_dir, logs_dir)

        env_vars = EnvVarsDict()

        # Backend databases
        env_vars |= {
            "CLP_REDIS_BACKEND_DB_COMPRESSION": str(
                self._clp_config.redis.compression_backend_database
            ),
            "CLP_REDIS_BACKEND_DB_QUERY": str(self._clp_config.redis.query_backend_database),
        }

        # Connection config
        env_vars |= {
            "CLP_REDIS_HOST": _get_ip_from_hostname(self._clp_config.redis.host),
            "CLP_REDIS_PORT": str(self._clp_config.redis.port),
        }

        # Credentials
        env_vars |= {
            "CLP_REDIS_PASS": self._clp_config.redis.password,
        }

        # Paths
        env_vars |= {
            "CLP_REDIS_CONF_FILE_HOST": str(conf_file),
            "CLP_REDIS_DATA_DIR_HOST": str(data_dir),
            "CLP_REDIS_LOGS_DIR_HOST": str(logs_dir),
        }

        return env_vars

    def _set_up_env_for_spider_db(self) -> EnvVarsDict:
        """
        Sets up environment variables for the Spider database component.

        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = "spider_db"
        logger.info(f"Setting up environment for {component_name}...")

        env_vars = EnvVarsDict()

        # Database
        env_vars |= {
            "SPIDER_DB_URL": self._clp_config.spider_db.get_url(),
        }

        # Credentials
        env_vars |= {
            "SPIDER_DB_USER": self._clp_config.spider_db.username,
            "SPIDER_DB_PASS": self._clp_config.spider_db.password,
        }

        return env_vars

    def _set_up_env_for_spider_scheduler(self) -> EnvVarsDict:
        """
        Sets up environment variables for the Spider database component.

        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = SPIDER_SCHEDULER_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        env_vars = EnvVarsDict()

        # Connection config
        env_vars |= {
            "SPIDER_SCHEDULER_HOST": _get_ip_from_hostname(self._clp_config.spider_scheduler.host),
            "SPIDER_SCHEDULER_PORT": str(self._clp_config.spider_scheduler.port),
        }

        return env_vars

    def _set_up_env_for_results_cache(self) -> EnvVarsDict:
        """
        Sets up environment variables and directories for the results cache (MongoDB) component.

        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = RESULTS_CACHE_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        conf_file = self._conf_dir / "mongo" / "mongod.conf"
        data_dir = self._clp_config.data_directory / component_name
        logs_dir = self._clp_config.logs_directory / component_name
        validate_results_cache_config(self._clp_config, conf_file, data_dir, logs_dir)

        data_dir.mkdir(exist_ok=True, parents=True)
        logs_dir.mkdir(exist_ok=True, parents=True)
        _chown_paths_if_root(data_dir, logs_dir)

        env_vars = EnvVarsDict()

        # Collections
        env_vars |= {
            "CLP_RESULTS_CACHE_STREAM_COLLECTION_NAME": (
                self._clp_config.results_cache.stream_collection_name
            ),
        }

        # Connection config
        env_vars |= {
            "CLP_RESULTS_CACHE_DB_NAME": self._clp_config.results_cache.db_name,
            "CLP_RESULTS_CACHE_HOST": _get_ip_from_hostname(self._clp_config.results_cache.host),
            "CLP_RESULTS_CACHE_PORT": str(self._clp_config.results_cache.port),
        }

        # Paths
        env_vars |= {
            "CLP_RESULTS_CACHE_CONF_FILE_HOST": str(conf_file),
            "CLP_RESULTS_CACHE_DATA_DIR_HOST": str(data_dir),
            "CLP_RESULTS_CACHE_LOGS_DIR_HOST": str(logs_dir),
        }

        return env_vars

    def _set_up_env_for_compression_scheduler(self) -> EnvVarsDict:
        """
        Sets up environment variables and files for the compression scheduler component.

        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = COMPRESSION_SCHEDULER_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        logs_dir = self._clp_config.logs_directory / component_name
        logs_dir.mkdir(parents=True, exist_ok=True)

        env_vars = EnvVarsDict()

        # Logging config
        env_vars |= {
            "CLP_COMPRESSION_SCHEDULER_LOGGING_LEVEL": (
                self._clp_config.compression_scheduler.logging_level
            ),
        }

        return env_vars

    def _set_up_env_for_query_scheduler(self) -> EnvVarsDict:
        """
        Sets up environment variables and files for the query scheduler component.

        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = QUERY_SCHEDULER_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        logs_dir = self._clp_config.logs_directory / component_name
        logs_dir.mkdir(parents=True, exist_ok=True)

        env_vars = EnvVarsDict()

        # Logging config
        env_vars |= {
            "CLP_QUERY_SCHEDULER_LOGGING_LEVEL": self._clp_config.query_scheduler.logging_level,
        }

        return env_vars

    def _set_up_env_for_compression_worker(self, num_workers: int) -> EnvVarsDict:
        """
        Sets up environment variables for the compression worker component.

        :param num_workers: Number of worker processes to run.
        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = COMPRESSION_WORKER_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        logs_dir = self._clp_config.logs_directory / component_name
        logs_dir.mkdir(parents=True, exist_ok=True)

        env_vars = EnvVarsDict()

        # Logging config
        env_vars |= {
            "CLP_COMPRESSION_WORKER_LOGGING_LEVEL": (
                self._clp_config.compression_worker.logging_level
            ),
        }

        # Resources
        env_vars |= {
            "CLP_COMPRESSION_WORKER_CONCURRENCY": str(num_workers),
        }

        return env_vars

    def _set_up_env_for_query_worker(self, num_workers: int) -> EnvVarsDict:
        """
        Sets up environment variables for the query worker component.

        :param num_workers: Number of worker processes to run.
        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = QUERY_WORKER_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        logs_dir = self._clp_config.logs_directory / component_name
        logs_dir.mkdir(parents=True, exist_ok=True)

        env_vars = EnvVarsDict()

        # Logging config
        env_vars |= {
            "CLP_QUERY_WORKER_LOGGING_LEVEL": self._clp_config.query_worker.logging_level,
        }

        # Resources
        env_vars |= {
            "CLP_QUERY_WORKER_CONCURRENCY": str(num_workers),
        }

        return env_vars

    def _set_up_env_for_reducer(self, num_workers: int) -> EnvVarsDict:
        """
        Sets up environment variables for the reducer component.

        :param num_workers: Number of worker processes to run.
        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = REDUCER_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        logs_dir = self._clp_config.logs_directory / component_name
        logs_dir.mkdir(parents=True, exist_ok=True)

        env_vars = EnvVarsDict()

        # Logging config
        env_vars |= {
            "CLP_REDUCER_LOGGING_LEVEL": self._clp_config.reducer.logging_level,
        }

        # Resources
        env_vars |= {
            "CLP_REDUCER_CONCURRENCY": str(num_workers),
            "CLP_REDUCER_UPSERT_INTERVAL": str(self._clp_config.reducer.upsert_interval),
        }

        return env_vars

    def _set_up_env_for_webui(self, container_clp_config: CLPConfig) -> EnvVarsDict:
        """
        Sets up environment variables and settings for the Web UI component.

        :param container_clp_config: CLP configuration inside the containers.
        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = WEBUI_COMPONENT_NAME
        logger.info(f"Setting up environment for {component_name}...")

        container_webui_dir = CONTAINER_CLP_HOME / "var" / "www" / "webui"
        client_settings_json_path = (
            self._clp_home / "var" / "www" / "webui" / "client" / "settings.json"
        )
        server_settings_json_path = (
            self._clp_home / "var" / "www" / "webui" / "server" / "dist" / "settings.json"
        )
        validate_webui_config(
            self._clp_config, client_settings_json_path, server_settings_json_path
        )

        # Read, update, and write back client's and server's settings.json
        clp_db_connection_params = self._clp_config.database.get_clp_connection_params_and_type(
            True
        )
        table_prefix = clp_db_connection_params["table_prefix"]
        if StorageEngine.CLP_S == self._clp_config.package.storage_engine:
            archives_table_name = ""
            files_table_name = ""
        else:
            archives_table_name = get_archives_table_name(table_prefix, None)
            files_table_name = get_files_table_name(table_prefix, None)

        client_settings_json_updates = {
            "ClpStorageEngine": self._clp_config.package.storage_engine,
            "ClpQueryEngine": self._clp_config.package.query_engine,
            "MongoDbSearchResultsMetadataCollectionName": (
                self._clp_config.webui.results_metadata_collection_name
            ),
            "SqlDbClpArchivesTableName": archives_table_name,
            "SqlDbClpDatasetsTableName": get_datasets_table_name(table_prefix),
            "SqlDbClpFilesTableName": files_table_name,
            "SqlDbClpTablePrefix": table_prefix,
            "SqlDbCompressionJobsTableName": COMPRESSION_JOBS_TABLE_NAME,
        }
        client_settings_json = self._read_and_update_settings_json(
            client_settings_json_path, client_settings_json_updates
        )
        with open(client_settings_json_path, "w") as client_settings_json_file:
            client_settings_json_file.write(json.dumps(client_settings_json))

        server_settings_json_updates = {
            "SqlDbHost": container_clp_config.database.host,
            "SqlDbPort": container_clp_config.database.port,
            "SqlDbName": self._clp_config.database.name,
            "SqlDbQueryJobsTableName": QUERY_JOBS_TABLE_NAME,
            "MongoDbHost": container_clp_config.results_cache.host,
            "MongoDbPort": container_clp_config.results_cache.port,
            "MongoDbName": self._clp_config.results_cache.db_name,
            "MongoDbSearchResultsMetadataCollectionName": (
                self._clp_config.webui.results_metadata_collection_name
            ),
            "MongoDbStreamFilesCollectionName": (
                self._clp_config.results_cache.stream_collection_name
            ),
            "ClientDir": str(container_webui_dir / "client"),
            "LogViewerDir": str(container_webui_dir / "yscope-log-viewer"),
            "StreamTargetUncompressedSize": self._clp_config.stream_output.target_uncompressed_size,
            "ClpQueryEngine": self._clp_config.package.query_engine,
        }

        stream_storage = self._clp_config.stream_output.storage
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

        query_engine = self._clp_config.package.query_engine
        if QueryEngine.PRESTO == query_engine:
            server_settings_json_updates["PrestoHost"] = self._clp_config.presto.host
            server_settings_json_updates["PrestoPort"] = self._clp_config.presto.port
        else:
            server_settings_json_updates["PrestoHost"] = None
            server_settings_json_updates["PrestoPort"] = None

        server_settings_json = self._read_and_update_settings_json(
            server_settings_json_path, server_settings_json_updates
        )
        with open(server_settings_json_path, "w") as settings_json_file:
            settings_json_file.write(json.dumps(server_settings_json))

        env_vars = EnvVarsDict()

        # Connection config
        env_vars |= {
            "CLP_WEBUI_HOST": _get_ip_from_hostname(self._clp_config.webui.host),
            "CLP_WEBUI_PORT": str(self._clp_config.webui.port),
        }

        # Security config
        env_vars |= {
            "CLP_WEBUI_RATE_LIMIT": str(self._clp_config.webui.rate_limit),
        }

        return env_vars

    def _set_up_env_for_garbage_collector(self) -> EnvVarsDict:
        """
        Sets up environment variables for the garbage collector component.

        :return: Dictionary of environment variables necessary to launch the component.
        """
        component_name = GARBAGE_COLLECTOR_COMPONENT_NAME
        if not is_retention_period_configured(self._clp_config):
            logger.info(
                f"Retention period is not configured, skipping {component_name} creation..."
            )
            return EnvVarsDict(
                {
                    "CLP_GARBAGE_COLLECTOR_ENABLED": "0",
                }
            )

        logger.info(f"Setting up environment for {component_name}...")

        logs_dir = self._clp_config.logs_directory / component_name
        logs_dir.mkdir(parents=True, exist_ok=True)

        env_vars = EnvVarsDict()

        # Logging config
        env_vars |= {
            "CLP_GARBAGE_COLLECTOR_LOGGING_LEVEL": self._clp_config.garbage_collector.logging_level
        }

        return env_vars

    def _read_and_update_settings_json(
        self, settings_file_path: pathlib.Path, updates: dict[str, Any]
    ) -> dict[str, Any]:
        """
        Reads and updates a settings JSON file.

        :param settings_file_path:
        :param updates:
        """
        with open(settings_file_path, "r") as settings_json_file:
            settings_object = json.loads(settings_json_file.read())
        self._update_settings_object("", settings_object, updates)

        return settings_object

    def _update_settings_object(
        self,
        parent_key_prefix: str,
        settings: dict[str, Any],
        updates: dict[str, Any],
    ) -> None:
        """
        Recursively updates the given settings object with the values from `updates`.

        :param parent_key_prefix: The prefix for keys at this level in the settings dictionary.
        :param settings: The settings to update.
        :param updates: The updates.
        :raise ValueError: If a key in `updates` doesn't exist in `settings`.
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
                settings[key] = value


class DockerComposeController(BaseController):
    """
    Controller for orchestrating CLP components using Docker Compose.
    """

    def __init__(self, clp_config: CLPConfig, instance_id: str) -> None:
        self._project_name = f"clp-package-{instance_id}"
        super().__init__(clp_config)

    def start(self) -> None:
        """
        Starts CLP's components using Docker Compose.

        :raise: Propagates `check_docker_dependencies`'s exceptions.
        :raise: Propagates `subprocess.run`'s exceptions.
        """
        check_docker_dependencies(
            should_compose_project_be_running=False, project_name=self._project_name
        )
        self._set_up_env()

        deployment_type = self._clp_config.get_deployment_type()
        logger.info(f"Starting CLP using Docker Compose ({deployment_type} deployment)...")

        cmd = ["docker", "compose", "--project-name", self._project_name]
        if deployment_type == DeploymentType.BASE:
            if self._clp_config.compression_scheduler.type == OrchestrationType.spider:
                cmd += ["--file", "docker-compose.spider.base.yaml"]
            else:
                cmd += ["--file", "docker-compose.base.yaml"]
        elif self._clp_config.compression_scheduler.type == OrchestrationType.spider:
            cmd += ["--file", "docker-compose.spider.yaml"]
        cmd += ["up", "--detach", "--wait"]
        subprocess.run(
            cmd,
            cwd=self._clp_home,
            check=True,
        )
        logger.info("Started CLP.")

    def stop(self) -> None:
        """
        Stops CLP components deployed via Docker Compose.

        :raise: Propagates `subprocess.run`'s exceptions.
        """
        try:
            check_docker_dependencies(
                should_compose_project_be_running=True, project_name=self._project_name
            )
        except DockerComposeProjectNotRunningError:
            logger.info(
                "Docker Compose project '%s' is not running. Nothing to stop.",
                self._project_name,
            )
            return
        except DockerDependencyError as e:
            logger.warning(
                'Docker dependencies check failed: "%s". Attempting to stop CLP containers '
                "anyway...",
                e,
            )
        else:
            logger.info("Stopping all CLP containers using Docker Compose...")

        subprocess.run(
            ["docker", "compose", "--project-name", self._project_name, "down"],
            cwd=self._clp_home,
            check=True,
        )
        logger.info("Stopped CLP.")

    @staticmethod
    def _get_num_workers() -> int:
        """
        :return: Number of worker processes to run.
        """
        # This will change when we move from single to multi-container workers. See y-scope/clp#1424
        return multiprocessing.cpu_count() // 2

    def _set_up_env(self) -> None:
        # Generate container-specific config.
        container_clp_config = generate_docker_compose_container_config(self._clp_config)
        num_workers = self._get_num_workers()
        dump_shared_container_config(container_clp_config, self._clp_config)

        env_vars = EnvVarsDict()

        # Credentials
        if self._clp_config.stream_output.storage.type == StorageType.S3:
            stream_output_aws_auth = (
                self._clp_config.stream_output.storage.s3_config.aws_authentication
            )
            if stream_output_aws_auth.type == AwsAuthType.credentials:
                env_vars |= {
                    "CLP_STREAM_OUTPUT_AWS_ACCESS_KEY_ID": (
                        stream_output_aws_auth.credentials.access_key_id
                    ),
                    "CLP_STREAM_OUTPUT_AWS_SECRET_ACCESS_KEY": (
                        stream_output_aws_auth.credentials.secret_access_key
                    ),
                }

        # Identity config
        env_vars |= {
            "CLP_FIRST_PARTY_SERVICE_UID_GID": DEFAULT_UID_GID,
            "CLP_THIRD_PARTY_SERVICE_UID_GID": (
                THIRD_PARTY_SERVICE_UID_GID if os.geteuid() == 0 else DEFAULT_UID_GID
            ),
        }

        # Package config
        env_vars |= {
            "CLP_PACKAGE_CONTAINER_IMAGE_REF": self._clp_config.container_image_ref,
            "CLP_PACKAGE_STORAGE_ENGINE": self._clp_config.package.storage_engine,
        }

        # Paths
        aws_config_dir = self._clp_config.aws_config_directory
        env_vars |= {
            "CLP_AWS_CONFIG_DIR_HOST": (None if aws_config_dir is None else str(aws_config_dir)),
            "CLP_DATA_DIR_HOST": str(self._clp_config.data_directory),
            "CLP_LOGS_DIR_HOST": str(self._clp_config.logs_directory),
        }

        # Input config
        if self._clp_config.logs_input.type == StorageType.FS:
            env_vars["CLP_LOGS_INPUT_DIR_CONTAINER"] = str(
                container_clp_config.logs_input.directory
            )
            env_vars["CLP_LOGS_INPUT_DIR_HOST"] = str(self._clp_config.logs_input.directory)

        # Output config
        archive_output_dir_str = str(self._clp_config.archive_output.get_directory())
        stream_output_dir_str = str(self._clp_config.stream_output.get_directory())
        if self._clp_config.archive_output.storage.type == StorageType.FS:
            env_vars["CLP_ARCHIVE_OUTPUT_DIR_HOST"] = archive_output_dir_str
        if self._clp_config.archive_output.storage.type == StorageType.S3:
            env_vars["CLP_STAGED_ARCHIVE_OUTPUT_DIR_HOST"] = archive_output_dir_str
        if self._clp_config.stream_output.storage.type == StorageType.FS:
            env_vars["CLP_STREAM_OUTPUT_DIR_HOST"] = stream_output_dir_str
        if self._clp_config.stream_output.storage.type == StorageType.S3:
            env_vars["CLP_STAGED_STREAM_OUTPUT_DIR_HOST"] = stream_output_dir_str

        # Component-specific config
        env_vars |= self._set_up_env_for_database()
        env_vars |= self._set_up_env_for_queue()
        env_vars |= self._set_up_env_for_redis()
        if self._clp_config.compression_scheduler.type == OrchestrationType.spider:
            env_vars |= self._set_up_env_for_spider_db()
            env_vars |= self._set_up_env_for_spider_scheduler()
        env_vars |= self._set_up_env_for_results_cache()
        env_vars |= self._set_up_env_for_compression_scheduler()
        env_vars |= self._set_up_env_for_query_scheduler()
        env_vars |= self._set_up_env_for_compression_worker(num_workers)
        env_vars |= self._set_up_env_for_query_worker(num_workers)
        env_vars |= self._set_up_env_for_reducer(num_workers)
        env_vars |= self._set_up_env_for_webui(container_clp_config)
        env_vars |= self._set_up_env_for_garbage_collector()

        # Write the environment variables to the `.env` file.
        with open(f"{self._clp_home}/.env", "w") as env_file:
            for key, value in env_vars.items():
                if value is None:
                    continue
                env_file.write(f"{key}={value}\n")


def get_or_create_instance_id(clp_config: CLPConfig) -> str:
    """
    Gets or creates a unique instance ID for this CLP instance.

    :param clp_config:
    :return: The instance ID.
    """
    instance_id_file_path = clp_config.logs_directory / "instance-id"

    if instance_id_file_path.exists():
        with open(instance_id_file_path, "r") as f:
            instance_id = f.readline()
    else:
        instance_id = str(uuid.uuid4())[-4:]
        with open(instance_id_file_path, "w") as f:
            f.write(instance_id)

    return instance_id


def _chown_paths_if_root(*paths: pathlib.Path) -> None:
    """
    Changes ownership of the given paths to the default service container user/group IDs if the
    current process is running as root.

    :param paths:
    """
    if os.getuid() != 0:
        return
    for path in paths:
        _chown_recursively(path, THIRD_PARTY_SERVICE_UID, THIRD_PARTY_SERVICE_GID)


def _chown_recursively(
    path: pathlib.Path,
    user_id: int,
    group_id: int,
) -> None:
    """
    Recursively changes the owner of the given path to the given user ID and group ID.

    :param path:
    :param user_id:
    :param group_id:
    """
    chown_cmd = ["chown", "--recursive", f"{user_id}:{group_id}", str(path)]
    subprocess.run(chown_cmd, stdout=subprocess.DEVNULL, check=True)


def _get_ip_from_hostname(hostname: str) -> str:
    """
    Resolves a hostname to an IPv4 IP address.

    :param hostname:
    :return: The resolved IP address.
    """
    return socket.gethostbyname(hostname)
