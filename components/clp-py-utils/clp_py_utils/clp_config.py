import pathlib
import typing
from enum import auto

from pydantic import BaseModel, validator
from strenum import KebabCaseStrEnum

from .clp_logging import get_valid_logging_level, is_valid_logging_level
from .core import (
    get_config_value,
    make_config_path_absolute,
    read_yaml_config_file,
    validate_path_could_be_dir,
)

# Constants
# Component names
DB_COMPONENT_NAME = "database"
QUEUE_COMPONENT_NAME = "queue"
REDIS_COMPONENT_NAME = "redis"
RESULTS_CACHE_COMPONENT_NAME = "results_cache"
COMPRESSION_SCHEDULER_COMPONENT_NAME = "compression_scheduler"
SEARCH_SCHEDULER_COMPONENT_NAME = "search_scheduler"
COMPRESSION_WORKER_COMPONENT_NAME = "compression_worker"
SEARCH_WORKER_COMPONENT_NAME = "search_worker"
WEBUI_COMPONENT_NAME = "webui"

# Target names
ALL_TARGET_NAME = ""
CONTROLLER_TARGET_NAME = "controller"

SEARCH_JOBS_TABLE_NAME = "search_jobs"
COMPRESSION_JOBS_TABLE_NAME = "compression_jobs"
COMPRESSION_TASKS_TABLE_NAME = "compression_tasks"

CLP_DEFAULT_CREDENTIALS_FILE_PATH = pathlib.Path("etc") / "credentials.yml"
CLP_METADATA_TABLE_PREFIX = "clp_"


class StorageEngine(KebabCaseStrEnum):
    CLP = auto()
    CLP_S = auto()


VALID_STORAGE_ENGINES = [storage_engine.value for storage_engine in StorageEngine]


class Package(BaseModel):
    storage_engine: str = "clp"

    @validator("storage_engine")
    def validate_storage_engine(cls, field):
        if field not in VALID_STORAGE_ENGINES:
            raise ValueError(
                f"package.storage_engine must be one of the following"
                f" {'|'.join(VALID_STORAGE_ENGINES)}"
            )
        return field


class Database(BaseModel):
    type: str = "mariadb"
    host: str = "localhost"
    port: int = 3306
    name: str = "clp-db"
    ssl_cert: typing.Optional[str] = None
    auto_commit: bool = False
    compress: bool = True

    username: typing.Optional[str] = None
    password: typing.Optional[str] = None

    @validator("type")
    def validate_database_type(cls, field):
        supported_database_types = ["mysql", "mariadb"]
        if field not in supported_database_types:
            raise ValueError(
                f"database.type must be one of the following {'|'.join(supported_database_types)}"
            )
        return field

    @validator("name")
    def validate_database_name(cls, field):
        if "" == field:
            raise ValueError("database.name cannot be empty.")
        return field

    @validator("host")
    def validate_database_host(cls, field):
        if "" == field:
            raise ValueError("database.host cannot be empty.")
        return field

    def ensure_credentials_loaded(self):
        if self.username is None or self.password is None:
            raise ValueError("Credentials not loaded.")

    def get_mysql_connection_params(self, disable_localhost_socket_connection: bool = False):
        self.ensure_credentials_loaded()

        host = self.host
        if disable_localhost_socket_connection and "localhost" == self.host:
            host = "127.0.0.1"

        # Currently, mysql's connection parameters are the same as mariadb
        connection_params = {
            "host": host,
            "port": self.port,
            "user": self.username,
            "password": self.password,
            "database": self.name,
            "compress": self.compress,
            "autocommit": self.auto_commit,
        }
        if self.ssl_cert:
            connection_params["ssl_cert"] = self.ssl_cert
        return connection_params

    def get_clp_connection_params_and_type(self, disable_localhost_socket_connection: bool = False):
        self.ensure_credentials_loaded()

        host = self.host
        if disable_localhost_socket_connection and "localhost" == self.host:
            host = "127.0.0.1"

        connection_params_and_type = {
            # NOTE: clp-core does not distinguish between mysql and mariadb
            "type": "mysql",
            "host": host,
            "port": self.port,
            "username": self.username,
            "password": self.password,
            "name": self.name,
            "table_prefix": CLP_METADATA_TABLE_PREFIX,
            "compress": self.compress,
            "autocommit": self.auto_commit,
        }
        if self.ssl_cert:
            connection_params_and_type["ssl_cert"] = self.ssl_cert
        return connection_params_and_type


def _validate_logging_level(cls, field):
    if not is_valid_logging_level(field):
        raise ValueError(
            f"{cls.__name__}: '{field}' is not a valid logging level. Use one of"
            f" {get_valid_logging_level()}"
        )


class CompressionScheduler(BaseModel):
    jobs_poll_delay: float = 0.1  # seconds
    logging_level: str = "INFO"

    @validator("logging_level")
    def validate_logging_level(cls, field):
        _validate_logging_level(cls, field)
        return field


class SearchScheduler(BaseModel):
    jobs_poll_delay: float = 0.1  # seconds
    logging_level: str = "INFO"

    @validator("logging_level")
    def validate_logging_level(cls, field):
        _validate_logging_level(cls, field)
        return field


class CompressionWorker(BaseModel):
    logging_level: str = "INFO"

    @validator("logging_level")
    def validate_logging_level(cls, field):
        _validate_logging_level(cls, field)
        return field


class SearchWorker(BaseModel):
    logging_level: str = "INFO"

    @validator("logging_level")
    def validate_logging_level(cls, field):
        _validate_logging_level(cls, field)
        return field


class Redis(BaseModel):
    host: str = "localhost"
    port: int = 6379
    search_backend_database: int = 0
    compression_backend_database: int = 1
    # redis can perform authentication without a username
    password: typing.Optional[str]

    @validator("host")
    def validate_host(cls, field):
        if "" == field:
            raise ValueError(f"{REDIS_COMPONENT_NAME}.host cannot be empty.")
        return field


class ResultsCache(BaseModel):
    host: str = "localhost"
    port: int = 27017
    db_name: str = "clp-search"

    @validator("host")
    def validate_host(cls, field):
        if "" == field:
            raise ValueError(f"{RESULTS_CACHE_COMPONENT_NAME}.host cannot be empty.")
        return field

    @validator("db_name")
    def validate_db_name(cls, field):
        if "" == field:
            raise ValueError(f"{RESULTS_CACHE_COMPONENT_NAME}.db_name cannot be empty.")
        return field

    def get_uri(self):
        return f"mongodb://{self.host}:{self.port}/{self.db_name}"


class Queue(BaseModel):
    host: str = "localhost"
    port: int = 5672

    username: typing.Optional[str]
    password: typing.Optional[str]


class ArchiveOutput(BaseModel):
    directory: pathlib.Path = pathlib.Path("var") / "data" / "archives"
    target_archive_size: int = 256 * 1024 * 1024  # 256 MB
    target_dictionaries_size: int = 32 * 1024 * 1024  # 32 MB
    target_encoded_file_size: int = 256 * 1024 * 1024  # 256 MB
    target_segment_size: int = 256 * 1024 * 1024  # 256 MB

    @validator("target_archive_size")
    def validate_target_archive_size(cls, field):
        if field <= 0:
            raise ValueError("target_archive_size must be greater than 0")
        return field

    @validator("target_dictionaries_size")
    def validate_target_dictionaries_size(cls, field):
        if field <= 0:
            raise ValueError("target_dictionaries_size must be greater than 0")
        return field

    @validator("target_encoded_file_size")
    def validate_target_encoded_file_size(cls, field):
        if field <= 0:
            raise ValueError("target_encoded_file_size must be greater than 0")
        return field

    @validator("target_segment_size")
    def validate_target_segment_size(cls, field):
        if field <= 0:
            raise ValueError("target_segment_size must be greater than 0")
        return field

    def make_config_paths_absolute(self, clp_home: pathlib.Path):
        self.directory = make_config_path_absolute(clp_home, self.directory)

    def dump_to_primitive_dict(self):
        d = self.dict()
        # Turn directory (pathlib.Path) into a primitive string
        d["directory"] = str(d["directory"])
        return d


class WebUi(BaseModel):
    host: str = "localhost"
    port: int = 4000
    logging_level: str = "INFO"

    @validator("host")
    def validate_host(cls, field):
        if "" == field:
            raise ValueError(f"{WEBUI_COMPONENT_NAME}.host cannot be empty.")
        return field

    @validator("port")
    def validate_port(cls, field):
        min_valid_port = 0
        max_valid_port = 2**16 - 1
        if min_valid_port > field or max_valid_port < field:
            raise ValueError(
                f"{WEBUI_COMPONENT_NAME}.port is not within valid range "
                f"{min_valid_port}-{max_valid_port}."
            )
        return field

    @validator("logging_level")
    def validate_logging_level(cls, field):
        _validate_logging_level(cls, field)
        return field


class CLPConfig(BaseModel):
    execution_container: str = "ghcr.io/y-scope/clp/clp-execution-x86-ubuntu-focal:main"

    input_logs_directory: pathlib.Path = pathlib.Path("/")

    package: Package = Package()
    database: Database = Database()
    queue: Queue = Queue()
    redis: Redis = Redis()
    results_cache: ResultsCache = ResultsCache()
    compression_scheduler: CompressionScheduler = CompressionScheduler()
    search_scheduler: SearchScheduler = SearchScheduler()
    compression_worker: CompressionWorker = CompressionWorker()
    search_worker: SearchWorker = SearchWorker()
    webui: WebUi = WebUi()
    credentials_file_path: pathlib.Path = CLP_DEFAULT_CREDENTIALS_FILE_PATH

    archive_output: ArchiveOutput = ArchiveOutput()
    data_directory: pathlib.Path = pathlib.Path("var") / "data"
    logs_directory: pathlib.Path = pathlib.Path("var") / "log"

    def make_config_paths_absolute(self, clp_home: pathlib.Path):
        self.input_logs_directory = make_config_path_absolute(clp_home, self.input_logs_directory)
        self.credentials_file_path = make_config_path_absolute(clp_home, self.credentials_file_path)
        self.archive_output.make_config_paths_absolute(clp_home)
        self.data_directory = make_config_path_absolute(clp_home, self.data_directory)
        self.logs_directory = make_config_path_absolute(clp_home, self.logs_directory)

    def validate_input_logs_dir(self):
        # NOTE: This can't be a pydantic validator since input_logs_dir might be a package-relative
        # path that will only be resolved after pydantic validation
        input_logs_dir = self.input_logs_directory
        if not input_logs_dir.exists():
            raise ValueError(f"input_logs_directory '{input_logs_dir}' doesn't exist.")
        if not input_logs_dir.is_dir():
            raise ValueError(f"input_logs_directory '{input_logs_dir}' is not a directory.")

    def validate_archive_output_dir(self):
        try:
            validate_path_could_be_dir(self.archive_output.directory)
        except ValueError as ex:
            raise ValueError(f"archive_output.directory is invalid: {ex}")

    def validate_data_dir(self):
        try:
            validate_path_could_be_dir(self.data_directory)
        except ValueError as ex:
            raise ValueError(f"data_directory is invalid: {ex}")

    def validate_logs_dir(self):
        try:
            validate_path_could_be_dir(self.logs_directory)
        except ValueError as ex:
            raise ValueError(f"logs_directory is invalid: {ex}")

    def load_database_credentials_from_file(self):
        config = read_yaml_config_file(self.credentials_file_path)
        if config is None:
            raise ValueError(f"Credentials file '{self.credentials_file_path}' is empty.")
        try:
            self.database.username = get_config_value(config, f"{DB_COMPONENT_NAME}.user")
            self.database.password = get_config_value(config, f"{DB_COMPONENT_NAME}.password")
        except KeyError as ex:
            raise ValueError(
                f"Credentials file '{self.credentials_file_path}' does not contain key '{ex}'."
            )

    def load_queue_credentials_from_file(self):
        config = read_yaml_config_file(self.credentials_file_path)
        if config is None:
            raise ValueError(f"Credentials file '{self.credentials_file_path}' is empty.")
        try:
            self.queue.username = get_config_value(config, f"{QUEUE_COMPONENT_NAME}.user")
            self.queue.password = get_config_value(config, f"{QUEUE_COMPONENT_NAME}.password")
        except KeyError as ex:
            raise ValueError(
                f"Credentials file '{self.credentials_file_path}' does not contain key '{ex}'."
            )

    def load_redis_credentials_from_file(self):
        config = read_yaml_config_file(self.credentials_file_path)
        if config is None:
            raise ValueError(f"Credentials file '{self.credentials_file_path}' is empty.")
        try:
            self.redis.password = get_config_value(config, f"{REDIS_COMPONENT_NAME}.password")
        except KeyError as ex:
            raise ValueError(
                f"Credentials file '{self.credentials_file_path}' does not contain key '{ex}'."
            )

    def dump_to_primitive_dict(self):
        d = self.dict()
        d["archive_output"] = self.archive_output.dump_to_primitive_dict()
        # Turn paths into primitive strings
        d["input_logs_directory"] = str(self.input_logs_directory)
        d["credentials_file_path"] = str(self.credentials_file_path)
        d["data_directory"] = str(self.data_directory)
        d["logs_directory"] = str(self.logs_directory)
        return d
