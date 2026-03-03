import os
import pathlib
from enum import auto
from types import MappingProxyType
from typing import Annotated, Any, ClassVar, Literal
from urllib.parse import urlencode

import yaml
from pydantic import (
    BaseModel,
    ConfigDict,
    Field,
    field_validator,
    model_validator,
    PlainSerializer,
    PrivateAttr,
    StringConstraints,
)
from strenum import KebabCaseStrEnum, LowercaseStrEnum

from clp_py_utils.clp_logging import LoggingLevel
from clp_py_utils.core import (
    get_config_value,
    make_config_path_absolute,
    read_yaml_config_file,
    resolve_host_path_in_container,
    validate_path_could_be_dir,
)
from clp_py_utils.serialization_utils import serialize_path, serialize_str_enum

# Constants
# Component names
DB_COMPONENT_NAME = "database"
QUEUE_COMPONENT_NAME = "queue"
REDIS_COMPONENT_NAME = "redis"
SPIDER_SCHEDULER_COMPONENT_NAME = "spider_scheduler"
REDUCER_COMPONENT_NAME = "reducer"
RESULTS_CACHE_COMPONENT_NAME = "results_cache"
COMPRESSION_SCHEDULER_COMPONENT_NAME = "compression_scheduler"
QUERY_SCHEDULER_COMPONENT_NAME = "query_scheduler"
PRESTO_COORDINATOR_COMPONENT_NAME = "presto-coordinator"
COMPRESSION_WORKER_COMPONENT_NAME = "compression_worker"
QUERY_WORKER_COMPONENT_NAME = "query_worker"
API_SERVER_COMPONENT_NAME = "api_server"
LOG_INGESTOR_COMPONENT_NAME = "log_ingestor"
WEBUI_COMPONENT_NAME = "webui"
MCP_SERVER_COMPONENT_NAME = "mcp_server"
GARBAGE_COLLECTOR_COMPONENT_NAME = "garbage_collector"

# Action names
ARCHIVE_MANAGER_ACTION_NAME = "archive_manager"

QUERY_JOBS_TABLE_NAME = "query_jobs"
QUERY_TASKS_TABLE_NAME = "query_tasks"
COMPRESSION_JOBS_TABLE_NAME = "compression_jobs"
COMPRESSION_TASKS_TABLE_NAME = "compression_tasks"

# Paths
CONTAINER_CLP_HOME = pathlib.Path("/") / "opt" / "clp"
CONTAINER_AWS_CONFIG_DIRECTORY = CONTAINER_CLP_HOME / ".aws"
CONTAINER_INPUT_LOGS_ROOT_DIR = pathlib.Path("/") / "mnt" / "logs"
CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH = pathlib.Path("etc") / "clp-config.yaml"
CLP_DEFAULT_CREDENTIALS_FILE_PATH = pathlib.Path("etc") / "credentials.yaml"
CLP_DEFAULT_DATA_DIRECTORY_PATH = pathlib.Path("var") / "data"
CLP_DEFAULT_ARCHIVES_DIRECTORY_PATH = CLP_DEFAULT_DATA_DIRECTORY_PATH / "archives"
CLP_DEFAULT_ARCHIVES_STAGING_DIRECTORY_PATH = CLP_DEFAULT_DATA_DIRECTORY_PATH / "staged-archives"
CLP_DEFAULT_STREAMS_DIRECTORY_PATH = CLP_DEFAULT_DATA_DIRECTORY_PATH / "streams"
CLP_DEFAULT_STREAMS_STAGING_DIRECTORY_PATH = CLP_DEFAULT_DATA_DIRECTORY_PATH / "staged-streams"
CLP_DEFAULT_LOG_DIRECTORY_PATH = pathlib.Path("var") / "log"
CLP_DEFAULT_TMP_DIRECTORY_PATH = pathlib.Path("var") / "tmp"
CLP_DEFAULT_DATASET_NAME = "default"
CLP_METADATA_TABLE_PREFIX = "clp_"
CLP_PACKAGE_CONTAINER_IMAGE_ID_PATH = pathlib.Path("clp-package-image.id")
CLP_SHARED_CONFIG_FILENAME = ".clp-config.yaml"
CLP_VERSION_FILE_PATH = pathlib.Path("VERSION")

# Environment variable names
CLP_DB_ROOT_USER_ENV_VAR_NAME = "CLP_DB_ROOT_USER"
CLP_DB_ROOT_PASS_ENV_VAR_NAME = "CLP_DB_ROOT_PASS"
CLP_DB_USER_ENV_VAR_NAME = "CLP_DB_USER"
CLP_DB_PASS_ENV_VAR_NAME = "CLP_DB_PASS"
CLP_QUEUE_USER_ENV_VAR_NAME = "CLP_QUEUE_USER"
CLP_QUEUE_PASS_ENV_VAR_NAME = "CLP_QUEUE_PASS"
CLP_REDIS_PASS_ENV_VAR_NAME = "CLP_REDIS_PASS"
SPIDER_DB_USER_ENV_VAR_NAME = "SPIDER_DB_USER"
SPIDER_DB_PASS_ENV_VAR_NAME = "SPIDER_DB_PASS"

# Serializer
StrEnumSerializer = PlainSerializer(serialize_str_enum)
# Generic types
NonEmptyStr = Annotated[str, StringConstraints(min_length=1, strip_whitespace=True)]
NonNegativeInt = Annotated[int, Field(ge=0)]
PositiveFloat = Annotated[float, Field(gt=0)]
PositiveInt = Annotated[int, Field(gt=0)]
# Specific types
# TODO: Replace this with pydantic_extra_types.domain.DomainStr.
DomainStr = NonEmptyStr
Port = Annotated[int, Field(gt=0, lt=2**16)]
SerializablePath = Annotated[pathlib.Path, PlainSerializer(serialize_path)]
ZstdCompressionLevel = Annotated[int, Field(ge=1, le=19)]

LoggingLevelRust = Literal[
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE",
    "OFF",
]


class DeploymentType(KebabCaseStrEnum):
    BASE = auto()
    FULL = auto()
    SPIDER_BASE = auto()
    SPIDER_FULL = auto()


class StorageEngine(KebabCaseStrEnum):
    CLP = auto()
    CLP_S = auto()


StorageEngineStr = Annotated[StorageEngine, StrEnumSerializer]


class BundledService(LowercaseStrEnum):
    DATABASE = auto()
    QUEUE = auto()
    REDIS = auto()
    RESULTS_CACHE = auto()


BundledServiceStr = Annotated[BundledService, StrEnumSerializer]


class DatabaseEngine(KebabCaseStrEnum):
    MARIADB = auto()
    MYSQL = auto()


DatabaseEngineStr = Annotated[DatabaseEngine, StrEnumSerializer]


class OrchestrationType(KebabCaseStrEnum):
    CELERY = auto()
    SPIDER = auto()


OrchestrationTypeStr = Annotated[OrchestrationType, StrEnumSerializer]


class QueryEngine(KebabCaseStrEnum):
    CLP = auto()
    CLP_S = auto()
    PRESTO = auto()


QueryEngineStr = Annotated[QueryEngine, StrEnumSerializer]


class StorageType(LowercaseStrEnum):
    FS = auto()
    S3 = auto()


class AwsAuthType(LowercaseStrEnum):
    credentials = auto()
    profile = auto()
    env_vars = auto()
    ec2 = auto()


AwsAuthTypeStr = Annotated[AwsAuthType, StrEnumSerializer]


class Package(BaseModel):
    storage_engine: StorageEngineStr = StorageEngine.CLP_S
    query_engine: QueryEngineStr = QueryEngine.CLP_S

    @model_validator(mode="after")
    def validate_query_engine_package_compatibility(self):
        query_engine = self.query_engine
        storage_engine = self.storage_engine

        if query_engine in [QueryEngine.CLP, QueryEngine.CLP_S]:
            if query_engine != storage_engine:
                raise ValueError(
                    f"query_engine '{query_engine}' is only compatible with "
                    f"storage_engine '{query_engine}'."
                )
        elif query_engine == QueryEngine.PRESTO:
            if storage_engine != StorageEngine.CLP_S:
                raise ValueError(
                    f"query_engine '{QueryEngine.PRESTO}' is only compatible with "
                    f"storage_engine '{StorageEngine.CLP_S}'."
                )
        else:
            raise ValueError(f"Unsupported query_engine '{query_engine}'.")

        return self


class ClpDbUserType(KebabCaseStrEnum):
    """Database user types used by CLP components."""

    CLP = auto()
    ROOT = auto()
    SPIDER = auto()


class ClpDbNameType(KebabCaseStrEnum):
    """Database name types used by CLP components."""

    CLP = auto()
    SPIDER = auto()


_DB_USER_TYPE_TO_DB_NAME_TYPE: MappingProxyType[ClpDbUserType, ClpDbNameType] = MappingProxyType(
    {
        ClpDbUserType.CLP: ClpDbNameType.CLP,
        ClpDbUserType.ROOT: ClpDbNameType.CLP,
        ClpDbUserType.SPIDER: ClpDbNameType.SPIDER,
    }
)


yaml.SafeDumper.add_multi_representer(
    KebabCaseStrEnum,
    yaml.representer.SafeRepresenter.represent_str,
)


class DbUserCredentials(BaseModel):
    """Credentials for a database user."""

    username: NonEmptyStr
    password: NonEmptyStr


class Database(BaseModel):
    DEFAULT_PORT: ClassVar[int] = 3306

    type: DatabaseEngineStr = DatabaseEngine.MARIADB
    host: DomainStr = "localhost"
    port: Port = DEFAULT_PORT
    names: dict[ClpDbNameType, NonEmptyStr] = {
        ClpDbNameType.CLP: "clp-db",
        ClpDbNameType.SPIDER: "spider-db",
    }
    ssl_cert: NonEmptyStr | None = None
    auto_commit: bool = False
    compress: bool = True

    credentials: dict[ClpDbUserType, DbUserCredentials] = {}

    def ensure_credentials_loaded(self, user_type: ClpDbUserType) -> None:
        """
        Ensures that credentials for the given `user_type` are loaded.

        :param user_type:
        :raise ValueError: If credentials for the given `user_type` are not loaded.
        """
        if user_type not in self.credentials:
            err_msg = f"Credentials for user type '{user_type}' are not loaded."
            raise ValueError(err_msg)

    def get_mysql_connection_params(
        self,
        disable_localhost_socket_connection: bool = False,
        user_type: ClpDbUserType = ClpDbUserType.CLP,
    ) -> dict[str, Any]:
        """
        Returns a dictionary of connection parameters to be used by mysql's or mariadb's `connect()`
        method, ensuring only credentials for the given `user_type` are loaded.

        :param disable_localhost_socket_connection: If true, force TCP connections.
        :param user_type: User type whose credentials should be included.
        :return: Dictionary of MySQL connection parameters.
        """
        self.ensure_credentials_loaded(user_type)

        host = self.host
        if disable_localhost_socket_connection and "localhost" == self.host:
            host = "127.0.0.1"

        # Currently, mysql's connection parameters are the same as mariadb
        connection_params = {
            "host": host,
            "port": self.port,
            "user": self.credentials[user_type].username,
            "password": self.credentials[user_type].password,
            "database": self.names[_DB_USER_TYPE_TO_DB_NAME_TYPE[user_type]],
            "compress": self.compress,
            "autocommit": self.auto_commit,
        }
        if self.ssl_cert:
            connection_params["ssl_cert"] = self.ssl_cert
        return connection_params

    def get_clp_connection_params_and_type(
        self,
        disable_localhost_socket_connection: bool = False,
        user_type: ClpDbUserType = ClpDbUserType.CLP,
    ) -> dict[str, Any]:
        """
        Returns a dictionary of connection parameters to be used by CLP components and ensures only
        credentials for the given `user_type` are loaded.

        :param disable_localhost_socket_connection: If true, force TCP connections.
        :param user_type: User type whose credentials should be included.
        :return: Dictionary of CLP connection parameters.
        """
        self.ensure_credentials_loaded(user_type)

        host = self.host
        if disable_localhost_socket_connection and "localhost" == self.host:
            host = "127.0.0.1"

        d = self.dump_to_primitive_dict()

        d["credentials"] = {user_type: self.credentials[user_type].model_dump()}
        d["host"] = host
        d["table_prefix"] = CLP_METADATA_TABLE_PREFIX
        # NOTE: clp-core does not distinguish between mysql and mariadb
        d["type"] = DatabaseEngine.MYSQL.value

        return d

    def get_container_url(self, user_type: ClpDbUserType = ClpDbUserType.CLP) -> str:
        """
        Returns a JDBC URL for connecting to the database from within a container.
        """
        self.ensure_credentials_loaded(user_type)
        query = urlencode(
            {
                "user": self.credentials[user_type].username,
                "password": self.credentials[user_type].password,
            }
        )
        return (
            f"jdbc:{self.type.value}://{self.host}:{self.port}/"
            f"{self.names[_DB_USER_TYPE_TO_DB_NAME_TYPE[user_type]]}?{query}"
        )

    def dump_to_primitive_dict(self) -> dict[str, Any]:
        """:return: A dictionary representation of this model, excluding credentials."""
        return self.model_dump(exclude={"credentials"})

    def load_credentials_from_file(self, credentials_file_path: pathlib.Path):
        """
        Loads database credentials from a YAML file.

        :param credentials_file_path:
        :raise ValueError: If the file is empty or does not contain the expected keys.
        """
        config = read_yaml_config_file(credentials_file_path)
        if config is None:
            raise ValueError(f"Credentials file '{credentials_file_path}' is empty.")
        try:
            self.credentials[ClpDbUserType.CLP] = DbUserCredentials(
                username=get_config_value(config, f"{DB_COMPONENT_NAME}.username"),
                password=get_config_value(config, f"{DB_COMPONENT_NAME}.password"),
            )
            self.credentials[ClpDbUserType.ROOT] = DbUserCredentials(
                username=get_config_value(config, f"{DB_COMPONENT_NAME}.root_username"),
                password=get_config_value(config, f"{DB_COMPONENT_NAME}.root_password"),
            )
            self.credentials[ClpDbUserType.SPIDER] = DbUserCredentials(
                username=get_config_value(config, f"{DB_COMPONENT_NAME}.spider_username"),
                password=get_config_value(config, f"{DB_COMPONENT_NAME}.spider_password"),
            )
        except KeyError as ex:
            raise ValueError(
                f"Credentials file '{credentials_file_path}' does not contain key '{ex}'."
            )

    def load_credentials_from_env(self, user_type: ClpDbUserType = ClpDbUserType.CLP):
        """
        Loads database credentials from environment variables for the given user type.

        :param user_type:
        :raise ValueError: If the user type is not supported.
        :raise ValueError: Propagates `_get_env_var`'s exceptions.
        """
        if user_type == ClpDbUserType.CLP:
            user_env_var = CLP_DB_USER_ENV_VAR_NAME
            pass_env_var = CLP_DB_PASS_ENV_VAR_NAME
        elif user_type == ClpDbUserType.ROOT:
            user_env_var = CLP_DB_ROOT_USER_ENV_VAR_NAME
            pass_env_var = CLP_DB_ROOT_PASS_ENV_VAR_NAME
        elif user_type == ClpDbUserType.SPIDER:
            user_env_var = SPIDER_DB_USER_ENV_VAR_NAME
            pass_env_var = SPIDER_DB_PASS_ENV_VAR_NAME
        else:
            err_msg = f"Unsupported user type '{user_type}'."
            raise ValueError(err_msg)

        self.credentials[user_type] = DbUserCredentials(
            username=_get_env_var(user_env_var),
            password=_get_env_var(pass_env_var),
        )

    def transform_for_container(self, is_bundled: bool):
        self.host = DB_COMPONENT_NAME
        if is_bundled:
            self.port = self.DEFAULT_PORT


class SpiderScheduler(BaseModel):
    DEFAULT_PORT: ClassVar[int] = 6000

    host: DomainStr = "localhost"
    port: Port = DEFAULT_PORT

    def transform_for_container(self):
        self.host = SPIDER_SCHEDULER_COMPONENT_NAME
        self.port = self.DEFAULT_PORT


class CompressionScheduler(BaseModel):
    UNLIMITED_CONCURRENT_TASKS_PER_JOB: ClassVar[NonNegativeInt] = 0

    jobs_poll_delay: PositiveFloat = 0.1  # seconds
    max_concurrent_tasks_per_job: NonNegativeInt = UNLIMITED_CONCURRENT_TASKS_PER_JOB
    logging_level: LoggingLevel = "INFO"
    type: OrchestrationTypeStr = OrchestrationType.CELERY


class QueryScheduler(BaseModel):
    DEFAULT_PORT: ClassVar[int] = 7000

    host: DomainStr = "localhost"
    port: Port = DEFAULT_PORT
    jobs_poll_delay: PositiveFloat = 0.1  # seconds
    max_datasets_per_query: PositiveInt | None = 10
    num_archives_to_search_per_sub_job: PositiveInt = 16
    logging_level: LoggingLevel = "INFO"

    def transform_for_container(self):
        self.host = QUERY_SCHEDULER_COMPONENT_NAME
        self.port = self.DEFAULT_PORT


class CompressionWorker(BaseModel):
    logging_level: LoggingLevel = "INFO"


class QueryWorker(BaseModel):
    logging_level: LoggingLevel = "INFO"


class Redis(BaseModel):
    DEFAULT_PORT: ClassVar[int] = 6379

    host: DomainStr = "localhost"
    port: Port = DEFAULT_PORT
    query_backend_database: int = 0
    compression_backend_database: int = 1
    # redis can perform authentication without a username
    password: str | None = None

    def dump_to_primitive_dict(self):
        return self.model_dump(exclude={"password"})

    def load_credentials_from_file(self, credentials_file_path: pathlib.Path):
        config = read_yaml_config_file(credentials_file_path)
        if config is None:
            raise ValueError(f"Credentials file '{credentials_file_path}' is empty.")
        try:
            self.password = get_config_value(config, f"{REDIS_COMPONENT_NAME}.password")
        except KeyError as ex:
            raise ValueError(
                f"Credentials file '{credentials_file_path}' does not contain key '{ex}'."
            )

    def load_credentials_from_env(self):
        """
        :raise ValueError: if any expected environment variable is not set.
        """
        self.password = _get_env_var(CLP_REDIS_PASS_ENV_VAR_NAME)

    def transform_for_container(self, is_bundled: bool):
        self.host = REDIS_COMPONENT_NAME
        if is_bundled:
            self.port = self.DEFAULT_PORT


class Reducer(BaseModel):
    DEFAULT_PORT: ClassVar[int] = 14009

    host: DomainStr = "localhost"
    base_port: Port = DEFAULT_PORT
    logging_level: LoggingLevel = "INFO"
    upsert_interval: PositiveInt = 100  # milliseconds

    def transform_for_container(self):
        self.host = REDUCER_COMPONENT_NAME
        self.base_port = self.DEFAULT_PORT


class ResultsCache(BaseModel):
    DEFAULT_PORT: ClassVar[int] = 27017

    host: DomainStr = "localhost"
    port: Port = DEFAULT_PORT
    db_name: NonEmptyStr = "clp-query-results"
    stream_collection_name: NonEmptyStr = "stream-files"
    retention_period: PositiveInt | None = 60

    def get_uri(self):
        return f"mongodb://{self.host}:{self.port}/{self.db_name}"

    def transform_for_container(self, is_bundled: bool):
        self.host = RESULTS_CACHE_COMPONENT_NAME
        if is_bundled:
            self.port = self.DEFAULT_PORT


class Queue(BaseModel):
    DEFAULT_PORT: ClassVar[int] = 5672

    host: DomainStr = "localhost"
    port: Port = DEFAULT_PORT

    username: NonEmptyStr | None = None
    password: str | None = None

    def dump_to_primitive_dict(self):
        return self.model_dump(exclude={"username", "password"})

    def load_credentials_from_file(self, credentials_file_path: pathlib.Path):
        config = read_yaml_config_file(credentials_file_path)
        if config is None:
            raise ValueError(f"Credentials file '{credentials_file_path}' is empty.")
        try:
            self.username = get_config_value(config, f"{QUEUE_COMPONENT_NAME}.username")
            self.password = get_config_value(config, f"{QUEUE_COMPONENT_NAME}.password")
        except KeyError as ex:
            raise ValueError(
                f"Credentials file '{credentials_file_path}' does not contain key '{ex}'."
            )

    def load_credentials_from_env(self):
        """
        :raise ValueError: if any expected environment variable is not set.
        """
        self.username = _get_env_var(CLP_QUEUE_USER_ENV_VAR_NAME)
        self.password = _get_env_var(CLP_QUEUE_PASS_ENV_VAR_NAME)

    def transform_for_container(self, is_bundled: bool):
        self.host = QUEUE_COMPONENT_NAME
        if is_bundled:
            self.port = self.DEFAULT_PORT


class S3Credentials(BaseModel):
    access_key_id: NonEmptyStr
    secret_access_key: NonEmptyStr
    session_token: NonEmptyStr | None = None


class AwsAuthentication(BaseModel):
    type: AwsAuthTypeStr
    profile: NonEmptyStr | None = None
    credentials: S3Credentials | None = None

    @model_validator(mode="before")
    @classmethod
    def validate_authentication(cls, data):
        if not isinstance(data, dict):
            raise ValueError(
                "authentication config expects to be initialized from a dict, but received"
                f" {type(data).__name__}."
            )
        auth_type = data.get("type")
        profile = data.get("profile")
        credentials = data.get("credentials")

        try:
            auth_enum = AwsAuthType(auth_type)
        except ValueError:
            raise ValueError(f"Unsupported authentication type '{auth_type}'.")

        if profile and credentials:
            raise ValueError("profile and credentials cannot be set simultaneously.")
        if AwsAuthType.profile == auth_enum and not profile:
            raise ValueError(f"profile must be set when type is '{auth_enum}.'")
        if AwsAuthType.credentials == auth_enum and not credentials:
            raise ValueError(f"credentials must be set when type is '{auth_enum}.'")
        if auth_enum in [AwsAuthType.ec2, AwsAuthType.env_vars] and (profile or credentials):
            raise ValueError(f"profile and credentials must not be set when type is '{auth_enum}.'")
        return data


class S3Config(BaseModel):
    endpoint_url: str | None = None
    region_code: NonEmptyStr | None = None
    bucket: NonEmptyStr
    key_prefix: str
    aws_authentication: AwsAuthentication


class S3IngestionConfig(BaseModel):
    type: Literal[StorageType.S3.value] = StorageType.S3.value
    aws_authentication: AwsAuthentication

    def transform_for_container(self):
        pass


class FsStorage(BaseModel):
    type: Literal[StorageType.FS.value] = StorageType.FS.value
    directory: SerializablePath

    @field_validator("directory", mode="before")
    @classmethod
    def validate_directory(cls, value):
        _validate_directory(value)
        return value

    def make_config_paths_absolute(self, clp_home: pathlib.Path):
        self.directory = make_config_path_absolute(clp_home, self.directory)


class S3Storage(BaseModel):
    type: Literal[StorageType.S3.value] = StorageType.S3.value
    s3_config: S3Config
    staging_directory: SerializablePath

    @field_validator("staging_directory", mode="before")
    @classmethod
    def validate_staging_directory(cls, value):
        _validate_directory(value)
        return value

    @field_validator("s3_config")
    @classmethod
    def validate_key_prefix(cls, value):
        key_prefix = value.key_prefix
        if "" == key_prefix:
            raise ValueError("s3_config.key_prefix cannot be empty")
        if not key_prefix.endswith("/"):
            raise ValueError('s3_config.key_prefix must end with "/"')
        return value

    def make_config_paths_absolute(self, clp_home: pathlib.Path):
        self.staging_directory = make_config_path_absolute(clp_home, self.staging_directory)


class FsIngestionConfig(FsStorage):
    directory: SerializablePath = pathlib.Path("/")

    def transform_for_container(self):
        input_logs_dir = self.directory.resolve()
        self.directory = CONTAINER_INPUT_LOGS_ROOT_DIR / input_logs_dir.relative_to(
            input_logs_dir.anchor
        )


class ArchiveFsStorage(FsStorage):
    directory: SerializablePath = CLP_DEFAULT_ARCHIVES_DIRECTORY_PATH

    def transform_for_container(self):
        self.directory = pathlib.Path("/") / CLP_DEFAULT_ARCHIVES_DIRECTORY_PATH


class StreamFsStorage(FsStorage):
    directory: SerializablePath = CLP_DEFAULT_STREAMS_DIRECTORY_PATH

    def transform_for_container(self):
        self.directory = pathlib.Path("/") / CLP_DEFAULT_STREAMS_DIRECTORY_PATH


class ArchiveS3Storage(S3Storage):
    staging_directory: SerializablePath = CLP_DEFAULT_ARCHIVES_STAGING_DIRECTORY_PATH

    def transform_for_container(self):
        self.staging_directory = pathlib.Path("/") / CLP_DEFAULT_ARCHIVES_STAGING_DIRECTORY_PATH


class StreamS3Storage(S3Storage):
    staging_directory: SerializablePath = CLP_DEFAULT_STREAMS_STAGING_DIRECTORY_PATH

    def transform_for_container(self):
        self.staging_directory = pathlib.Path("/") / CLP_DEFAULT_STREAMS_STAGING_DIRECTORY_PATH


def _get_directory_from_storage_config(
    storage_config: FsStorage | S3Storage,
) -> pathlib.Path:
    storage_type = storage_config.type
    if StorageType.FS == storage_type:
        return storage_config.directory
    if StorageType.S3 == storage_type:
        return storage_config.staging_directory
    raise NotImplementedError(f"storage.type {storage_type} is not supported")


def _set_directory_for_storage_config(storage_config: FsStorage | S3Storage, directory) -> None:
    storage_type = storage_config.type
    if StorageType.FS == storage_type:
        storage_config.directory = directory
    elif StorageType.S3 == storage_type:
        storage_config.staging_directory = directory
    else:
        raise NotImplementedError(f"storage.type {storage_type} is not supported")


class ArchiveOutput(BaseModel):
    storage: ArchiveFsStorage | ArchiveS3Storage = ArchiveFsStorage()
    target_archive_size: PositiveInt = 256 * 1024 * 1024  # 256 MiB
    target_dictionaries_size: PositiveInt = 32 * 1024 * 1024  # 32 MiB
    target_encoded_file_size: PositiveInt = 256 * 1024 * 1024  # 256 MiB
    target_segment_size: PositiveInt = 256 * 1024 * 1024  # 256 MiB
    compression_level: ZstdCompressionLevel = 3
    retention_period: PositiveInt | None = None

    def set_directory(self, directory: pathlib.Path):
        _set_directory_for_storage_config(self.storage, directory)

    def get_directory(self) -> pathlib.Path:
        return _get_directory_from_storage_config(self.storage)


class StreamOutput(BaseModel):
    storage: StreamFsStorage | StreamS3Storage = StreamFsStorage()
    target_uncompressed_size: PositiveInt = 128 * 1024 * 1024

    def set_directory(self, directory: pathlib.Path):
        _set_directory_for_storage_config(self.storage, directory)

    def get_directory(self) -> pathlib.Path:
        return _get_directory_from_storage_config(self.storage)


class WebUi(BaseModel):
    DEFAULT_PORT: ClassVar[int] = 4000

    host: DomainStr = "localhost"
    port: Port = DEFAULT_PORT
    results_metadata_collection_name: NonEmptyStr = "results-metadata"
    rate_limit: PositiveInt = 1000


class SweepInterval(BaseModel):
    model_config = ConfigDict(extra="forbid")

    archive: PositiveInt = 60
    search_result: PositiveInt = 30


class McpServer(BaseModel):
    DEFAULT_PORT: ClassVar[int] = 8000

    host: DomainStr = "localhost"
    port: Port = DEFAULT_PORT
    logging_level: LoggingLevel = "INFO"


class GarbageCollector(BaseModel):
    logging_level: LoggingLevel = "INFO"
    sweep_interval: SweepInterval = SweepInterval()


class QueryJobPollingConfig(BaseModel):
    initial_backoff_ms: int = Field(default=100, alias="initial_backoff")
    max_backoff_ms: int = Field(default=5000, alias="max_backoff")


class ApiServer(BaseModel):
    host: DomainStr = "localhost"
    port: Port = 3001
    query_job_polling: QueryJobPollingConfig = QueryJobPollingConfig()
    default_max_num_query_results: int = 1000


class LogIngestor(BaseModel):
    host: DomainStr = "localhost"
    port: Port = 3002
    buffer_flush_timeout: PositiveInt = 300  # seconds
    buffer_flush_threshold: PositiveInt = 4096 * 1024 * 1024  # 4 GiB
    channel_capacity: PositiveInt = 10
    logging_level: LoggingLevelRust = "INFO"


class Presto(BaseModel):
    DEFAULT_PORT: ClassVar[int] = 8080

    host: DomainStr
    port: Port

    def transform_for_container(self):
        self.host = PRESTO_COORDINATOR_COMPONENT_NAME
        self.port = self.DEFAULT_PORT


def _get_env_var(name: str) -> str:
    value = os.getenv(name)
    if value is None:
        raise ValueError(f"Missing environment variable: {name}")
    return value


class ClpConfig(BaseModel):
    container_image_ref: NonEmptyStr | None = None

    logs_input: FsIngestionConfig | S3IngestionConfig = FsIngestionConfig()
    bundled: list[BundledServiceStr] = [
        BundledService.DATABASE,
        BundledService.QUEUE,
        BundledService.REDIS,
        BundledService.RESULTS_CACHE,
    ]

    package: Package = Package()
    database: Database = Database()
    # Default to use celery backend
    queue: Queue | None = Queue()
    redis: Redis | None = Redis()
    reducer: Reducer = Reducer()
    results_cache: ResultsCache = ResultsCache()
    compression_scheduler: CompressionScheduler = CompressionScheduler()
    spider_scheduler: SpiderScheduler | None = None
    query_scheduler: QueryScheduler = QueryScheduler()
    compression_worker: CompressionWorker = CompressionWorker()
    query_worker: QueryWorker = QueryWorker()
    webui: WebUi = WebUi()
    garbage_collector: GarbageCollector = GarbageCollector()
    api_server: ApiServer | None = ApiServer()
    log_ingestor: LogIngestor | None = LogIngestor()
    credentials_file_path: SerializablePath = CLP_DEFAULT_CREDENTIALS_FILE_PATH

    mcp_server: McpServer | None = None
    presto: Presto | None = None

    archive_output: ArchiveOutput = ArchiveOutput()
    stream_output: StreamOutput = StreamOutput()
    data_directory: SerializablePath = CLP_DEFAULT_DATA_DIRECTORY_PATH
    logs_directory: SerializablePath = CLP_DEFAULT_LOG_DIRECTORY_PATH
    tmp_directory: SerializablePath = CLP_DEFAULT_TMP_DIRECTORY_PATH
    aws_config_directory: SerializablePath | None = None

    _container_image_id_path: SerializablePath = PrivateAttr(
        default=CLP_PACKAGE_CONTAINER_IMAGE_ID_PATH
    )
    _version_file_path: SerializablePath = PrivateAttr(default=CLP_VERSION_FILE_PATH)

    @field_validator("aws_config_directory")
    @classmethod
    def expand_profile_user_home(cls, value: SerializablePath | None) -> SerializablePath | None:
        if value is not None:
            value = value.expanduser()
        return value

    def make_config_paths_absolute(self, clp_home: pathlib.Path):
        if StorageType.FS == self.logs_input.type:
            self.logs_input.make_config_paths_absolute(clp_home)
        self.credentials_file_path = make_config_path_absolute(clp_home, self.credentials_file_path)
        self.archive_output.storage.make_config_paths_absolute(clp_home)
        self.stream_output.storage.make_config_paths_absolute(clp_home)
        self.data_directory = make_config_path_absolute(clp_home, self.data_directory)
        self.logs_directory = make_config_path_absolute(clp_home, self.logs_directory)
        self.tmp_directory = make_config_path_absolute(clp_home, self.tmp_directory)
        self._container_image_id_path = make_config_path_absolute(
            clp_home, self._container_image_id_path
        )
        self._version_file_path = make_config_path_absolute(clp_home, self._version_file_path)

    def validate_logs_input_config(self, use_host_mount: bool = False):
        logs_input_type = self.logs_input.type
        if StorageType.FS == logs_input_type:
            # NOTE: This can't be a pydantic validator since input_logs_dir might be a
            # package-relative path that will only be resolved after pydantic validation
            input_logs_dir = self.logs_input.directory
            resolved_input_logs_dir = (
                resolve_host_path_in_container(input_logs_dir) if use_host_mount else input_logs_dir
            )
            if not resolved_input_logs_dir.exists():
                raise ValueError(f"logs_input.directory '{input_logs_dir}' doesn't exist.")
            if not resolved_input_logs_dir.is_dir():
                raise ValueError(f"logs_input.directory '{input_logs_dir}' is not a directory.")
        if StorageType.S3 == logs_input_type and StorageEngine.CLP_S != self.package.storage_engine:
            raise ValueError(
                f"logs_input.type = 's3' is only supported with package.storage_engine"
                f" = '{StorageEngine.CLP_S}'"
            )

    def validate_archive_output_config(self, use_host_mount: bool = False):
        if (
            StorageType.S3 == self.archive_output.storage.type
            and StorageEngine.CLP_S != self.package.storage_engine
        ):
            raise ValueError(
                f"archive_output.storage.type = 's3' is only supported with package.storage_engine"
                f" = '{StorageEngine.CLP_S}'"
            )
        archive_output_dir = self.archive_output.get_directory()
        resolved_archive_output_dir = (
            resolve_host_path_in_container(archive_output_dir)
            if use_host_mount
            else archive_output_dir
        )
        try:
            validate_path_could_be_dir(resolved_archive_output_dir)
        except ValueError as ex:
            raise ValueError(f"archive_output.storage's directory is invalid: {ex}")

    def validate_stream_output_config(self, use_host_mount: bool = False):
        if (
            StorageType.S3 == self.stream_output.storage.type
            and StorageEngine.CLP_S != self.package.storage_engine
        ):
            raise ValueError(
                f"stream_output.storage.type = 's3' is only supported with package.storage_engine"
                f" = '{StorageEngine.CLP_S}'"
            )
        stream_output_dir = self.stream_output.get_directory()
        resolved_stream_output_dir = (
            resolve_host_path_in_container(stream_output_dir)
            if use_host_mount
            else stream_output_dir
        )
        try:
            validate_path_could_be_dir(resolved_stream_output_dir)
        except ValueError as ex:
            raise ValueError(f"stream_output.storage's directory is invalid: {ex}")

    def validate_data_dir(self, use_host_mount: bool = False):
        data_dir = self.data_directory
        resolved_data_dir = resolve_host_path_in_container(data_dir) if use_host_mount else data_dir
        try:
            validate_path_could_be_dir(resolved_data_dir)
        except ValueError as ex:
            raise ValueError(f"data_directory is invalid: {ex}")

    def validate_logs_dir(self, use_host_mount: bool = False):
        logs_dir = self.logs_directory
        resolved_logs_dir = resolve_host_path_in_container(logs_dir) if use_host_mount else logs_dir
        try:
            validate_path_could_be_dir(resolved_logs_dir)
        except ValueError as ex:
            raise ValueError(f"logs_directory is invalid: {ex}")

    def validate_tmp_dir(self, use_host_mount: bool = False):
        tmp_dir = self.tmp_directory
        resolved_tmp_dir = resolve_host_path_in_container(tmp_dir) if use_host_mount else tmp_dir
        try:
            validate_path_could_be_dir(resolved_tmp_dir)
        except ValueError as ex:
            raise ValueError(f"tmp_directory is invalid: {ex}")

    def validate_aws_config_dir(self, use_host_mount: bool = False):
        profile_auth_used = False
        auth_configs = []

        if StorageType.S3 == self.logs_input.type:
            auth_configs.append(self.logs_input.aws_authentication)
        if StorageType.S3 == self.archive_output.storage.type:
            auth_configs.append(self.archive_output.storage.s3_config.aws_authentication)
        if StorageType.S3 == self.stream_output.storage.type:
            auth_configs.append(self.stream_output.storage.s3_config.aws_authentication)

        for auth in auth_configs:
            if AwsAuthType.profile == auth.type:
                profile_auth_used = True
                break

        if profile_auth_used:
            if self.aws_config_directory is None:
                raise ValueError(
                    "aws_config_directory must be set when using profile authentication"
                )
            resolved_aws_config_dir = (
                resolve_host_path_in_container(self.aws_config_directory)
                if use_host_mount
                else self.aws_config_directory
            )
            if not resolved_aws_config_dir.exists():
                raise ValueError(
                    f"aws_config_directory does not exist: '{self.aws_config_directory}'"
                )
        if not profile_auth_used and self.aws_config_directory is not None:
            raise ValueError(
                "aws_config_directory should not be set when profile authentication is not used"
            )

    def validate_api_server(self):
        if StorageEngine.CLP == self.package.storage_engine and self.api_server is not None:
            raise ValueError(
                f"The API server is only compatible with storage engine `{StorageEngine.CLP_S}`."
            )

    def load_container_image_ref(self):
        if self.container_image_ref is not None:
            # Accept configured value for debug purposes
            return

        if self._container_image_id_path.exists():
            with open(self._container_image_id_path) as image_id_file:
                self.container_image_ref = image_id_file.read().strip()
        else:
            with open(self._version_file_path) as version_file:
                clp_package_version = version_file.read().strip()
            self.container_image_ref = f"ghcr.io/y-scope/clp/clp-package:{clp_package_version}"

    def get_shared_config_file_path(self) -> pathlib.Path:
        return self.logs_directory / CLP_SHARED_CONFIG_FILENAME

    def get_deployment_type(self) -> DeploymentType:
        if OrchestrationType.SPIDER == self.compression_scheduler.type:
            if QueryEngine.PRESTO == self.package.query_engine:
                return DeploymentType.SPIDER_BASE
            return DeploymentType.SPIDER_FULL
        if QueryEngine.PRESTO == self.package.query_engine:
            return DeploymentType.BASE
        return DeploymentType.FULL

    def dump_to_primitive_dict(self):
        custom_serialized_fields = {"database", "queue", "redis"}
        d = self.model_dump(exclude=custom_serialized_fields)
        for key in custom_serialized_fields:
            value = getattr(self, key)
            d[key] = None if value is None else value.dump_to_primitive_dict()

        return d

    @model_validator(mode="after")
    def validate_log_ingestor_config(self):
        if self.log_ingestor is None:
            return self
        if self.package.storage_engine != StorageEngine.CLP_S:
            msg = f"log-ingestor is only compatible with storage engine `{StorageEngine.CLP_S}`."
            raise ValueError(msg)
        return self

    @model_validator(mode="after")
    def validate_presto_config(self):
        query_engine = self.package.query_engine
        presto = self.presto
        if query_engine == QueryEngine.PRESTO and presto is None:
            raise ValueError(
                f"`presto` config must be non-null when query_engine is `{query_engine}`"
            )
        return self

    @model_validator(mode="after")
    def validate_spider_config(self):
        orchestration_type = self.compression_scheduler.type
        if orchestration_type != OrchestrationType.SPIDER:
            return self
        if self.spider_scheduler is None:
            raise ValueError(
                "`spider_scheduler` must be configured when using Spider orchestration."
            )
        if self.database.type != DatabaseEngine.MARIADB:
            raise ValueError("Spider only supports MariaDB for the metadata database.")
        return self

    @model_validator(mode="after")
    def validate_celery_config(self):
        orchestration_type = self.compression_scheduler.type
        if orchestration_type != OrchestrationType.CELERY:
            return self
        if self.queue is None:
            raise ValueError("`queue` must be configured when using Celery orchestration.")
        if self.redis is None:
            raise ValueError("`redis` must be configured when using Celery orchestration.")
        return self

    def transform_for_container(self):
        """
        Converts all relevant directories to absolute paths inside the container, and updates
        component hostnames and ports to their container service names and default ports.
        """
        self.data_directory = pathlib.Path("/") / CLP_DEFAULT_DATA_DIRECTORY_PATH
        self.logs_directory = pathlib.Path("/") / CLP_DEFAULT_LOG_DIRECTORY_PATH
        self.tmp_directory = pathlib.Path("/") / CLP_DEFAULT_TMP_DIRECTORY_PATH
        if self.aws_config_directory is not None:
            self.aws_config_directory = CONTAINER_AWS_CONFIG_DIRECTORY
        self.logs_input.transform_for_container()
        self.archive_output.storage.transform_for_container()
        self.stream_output.storage.transform_for_container()

        self.database.transform_for_container(BundledService.DATABASE in self.bundled)
        if self.queue is not None:
            self.queue.transform_for_container(BundledService.QUEUE in self.bundled)
        if self.redis is not None:
            self.redis.transform_for_container(BundledService.REDIS in self.bundled)
        if self.spider_scheduler is not None:
            self.spider_scheduler.transform_for_container()
        self.results_cache.transform_for_container(BundledService.RESULTS_CACHE in self.bundled)
        self.query_scheduler.transform_for_container()
        self.reducer.transform_for_container()
        if self.package.query_engine == QueryEngine.PRESTO and self.presto is not None:
            self.presto.transform_for_container()


class WorkerConfig(BaseModel):
    package: Package = Package()
    archive_output: ArchiveOutput = ArchiveOutput()
    tmp_directory: SerializablePath = ClpConfig().tmp_directory

    # Only needed by query workers.
    stream_output: StreamOutput = StreamOutput()
    stream_collection_name: str = ResultsCache().stream_collection_name


def _validate_directory(value: Any):
    """
    Validates that the given value represents a directory path.

    :param value:
    :raise ValueError: if the value is not of type str.
    :raise ValueError: if the value is an empty string.
    """
    if not isinstance(value, str):
        raise ValueError("must be a string.")

    if "" == value.strip():
        raise ValueError("cannot be empty")
