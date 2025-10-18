import os
import pathlib
from enum import auto
from typing import Annotated, Any, Literal, Optional, Set, Union

from dotenv import dotenv_values
from pydantic import (
    BaseModel,
    ConfigDict,
    Field,
    field_validator,
    model_validator,
    PlainSerializer,
    PrivateAttr,
)
from strenum import KebabCaseStrEnum, LowercaseStrEnum

from .clp_logging import LoggingLevel
from .core import (
    get_config_value,
    make_config_path_absolute,
    read_yaml_config_file,
    validate_path_could_be_dir,
)
from .serialization_utils import serialize_enum, serialize_path

# Constants
# Component names
DB_COMPONENT_NAME = "database"
QUEUE_COMPONENT_NAME = "queue"
REDIS_COMPONENT_NAME = "redis"
REDUCER_COMPONENT_NAME = "reducer"
RESULTS_CACHE_COMPONENT_NAME = "results_cache"
COMPRESSION_SCHEDULER_COMPONENT_NAME = "compression_scheduler"
QUERY_SCHEDULER_COMPONENT_NAME = "query_scheduler"
COMPRESSION_WORKER_COMPONENT_NAME = "compression_worker"
QUERY_WORKER_COMPONENT_NAME = "query_worker"
WEBUI_COMPONENT_NAME = "webui"
GARBAGE_COLLECTOR_COMPONENT_NAME = "garbage_collector"

# Component groups
GENERAL_SCHEDULING_COMPONENTS = {
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
}
COMPRESSION_COMPONENTS = GENERAL_SCHEDULING_COMPONENTS | {
    DB_COMPONENT_NAME,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
}
QUERY_COMPONENTS = GENERAL_SCHEDULING_COMPONENTS | {
    DB_COMPONENT_NAME,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
}
UI_COMPONENTS = {
    RESULTS_CACHE_COMPONENT_NAME,
    WEBUI_COMPONENT_NAME,
}
STORAGE_MANAGEMENT_COMPONENTS = {GARBAGE_COLLECTOR_COMPONENT_NAME}
ALL_COMPONENTS = (
    COMPRESSION_COMPONENTS | QUERY_COMPONENTS | UI_COMPONENTS | STORAGE_MANAGEMENT_COMPONENTS
)

# Target names
ALL_TARGET_NAME = ""
CONTROLLER_TARGET_NAME = "controller"

TARGET_TO_COMPONENTS = {
    ALL_TARGET_NAME: ALL_COMPONENTS,
    CONTROLLER_TARGET_NAME: GENERAL_SCHEDULING_COMPONENTS
    | {
        COMPRESSION_SCHEDULER_COMPONENT_NAME,
        QUERY_SCHEDULER_COMPONENT_NAME,
    }
    | STORAGE_MANAGEMENT_COMPONENTS,
}

# Action names
ARCHIVE_MANAGER_ACTION_NAME = "archive_manager"

QUERY_JOBS_TABLE_NAME = "query_jobs"
QUERY_TASKS_TABLE_NAME = "query_tasks"
COMPRESSION_JOBS_TABLE_NAME = "compression_jobs"
COMPRESSION_TASKS_TABLE_NAME = "compression_tasks"

CLP_DEFAULT_CREDENTIALS_FILE_PATH = pathlib.Path("etc") / "credentials.yml"
CLP_DEFAULT_DATA_DIRECTORY_PATH = pathlib.Path("var") / "data"
CLP_DEFAULT_DATASET_NAME = "default"
CLP_METADATA_TABLE_PREFIX = "clp_"
CLP_PACKAGE_CONTAINER_IMAGE_ID_PATH = pathlib.Path("clp-package-image.id")
CLP_SHARED_CONFIG_FILENAME = ".clp-config.yml"
CLP_VERSION_FILE_PATH = pathlib.Path("VERSION")

# Environment variable names
CLP_DB_USER_ENV_VAR_NAME = "CLP_DB_USER"
CLP_DB_PASS_ENV_VAR_NAME = "CLP_DB_PASS"
CLP_QUEUE_USER_ENV_VAR_NAME = "CLP_QUEUE_USER"
CLP_QUEUE_PASS_ENV_VAR_NAME = "CLP_QUEUE_PASS"
CLP_REDIS_PASS_ENV_VAR_NAME = "CLP_REDIS_PASS"

# Generic types
NonEmptyStr = Annotated[str, Field(min_length=1)]
PositiveFloat = Annotated[float, Field(gt=0)]
PositiveInt = Annotated[int, Field(gt=0)]
# Specific types
# TODO: Replace this with pydantic_extra_types.domain.DomainStr.
DomainStr = NonEmptyStr
Port = Annotated[int, Field(gt=0, lt=2**16)]
ZstdCompressionLevel = Annotated[int, Field(ge=1, le=19)]

StrEnumSerializer = PlainSerializer(serialize_enum)

PathStr = Annotated[pathlib.Path, PlainSerializer(serialize_path)]


class StorageEngine(KebabCaseStrEnum):
    CLP = auto()
    CLP_S = auto()


StorageEngineStr = Annotated[StorageEngine, StrEnumSerializer]


class DatabaseEngine(KebabCaseStrEnum):
    MARIADB = auto()
    MYSQL = auto()


DatabaseEngineStr = Annotated[DatabaseEngine, StrEnumSerializer]


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
    storage_engine: StorageEngineStr = StorageEngine.CLP
    query_engine: QueryEngineStr = QueryEngine.CLP

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


class Database(BaseModel):
    type: DatabaseEngineStr = DatabaseEngine.MARIADB
    host: DomainStr = "localhost"
    port: Port = 3306
    name: NonEmptyStr = "clp-db"
    ssl_cert: Optional[NonEmptyStr] = None
    auto_commit: bool = False
    compress: bool = True

    username: Optional[str] = None
    password: Optional[str] = None

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
            "type": DatabaseEngine.MYSQL.value,
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

    def dump_to_primitive_dict(self):
        d = self.model_dump(exclude={"username", "password"})
        return d

    def load_credentials_from_file(self, credentials_file_path: pathlib.Path):
        config = read_yaml_config_file(credentials_file_path)
        if config is None:
            raise ValueError(f"Credentials file '{credentials_file_path}' is empty.")
        try:
            self.username = get_config_value(config, f"{DB_COMPONENT_NAME}.user")
            self.password = get_config_value(config, f"{DB_COMPONENT_NAME}.password")
        except KeyError as ex:
            raise ValueError(
                f"Credentials file '{credentials_file_path}' does not contain key '{ex}'."
            )

    def load_credentials_from_env(self):
        """
        :raise ValueError: if any expected environment variable is not set.
        """
        self.username = _get_env_var(CLP_DB_USER_ENV_VAR_NAME)
        self.password = _get_env_var(CLP_DB_PASS_ENV_VAR_NAME)


class CompressionScheduler(BaseModel):
    jobs_poll_delay: PositiveFloat = 0.1  # seconds
    logging_level: LoggingLevel = "INFO"


class QueryScheduler(BaseModel):
    host: DomainStr = "localhost"
    port: Port = 7000
    jobs_poll_delay: PositiveFloat = 0.1  # seconds
    num_archives_to_search_per_sub_job: PositiveInt = 16
    logging_level: LoggingLevel = "INFO"


class CompressionWorker(BaseModel):
    logging_level: LoggingLevel = "INFO"


class QueryWorker(BaseModel):
    logging_level: LoggingLevel = "INFO"


class Redis(BaseModel):
    host: DomainStr = "localhost"
    port: Port = 6379
    query_backend_database: int = 0
    compression_backend_database: int = 1
    # redis can perform authentication without a username
    password: Optional[str] = None

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


class Reducer(BaseModel):
    host: DomainStr = "localhost"
    base_port: Port = 14009
    logging_level: LoggingLevel = "INFO"
    upsert_interval: PositiveInt = 100  # milliseconds


class ResultsCache(BaseModel):
    host: DomainStr = "localhost"
    port: Port = 27017
    db_name: NonEmptyStr = "clp-query-results"
    stream_collection_name: NonEmptyStr = "stream-files"
    retention_period: Optional[PositiveInt] = 60

    def get_uri(self):
        return f"mongodb://{self.host}:{self.port}/{self.db_name}"


class Queue(BaseModel):
    host: DomainStr = "localhost"
    port: Port = 5672

    username: Optional[NonEmptyStr] = None
    password: Optional[str] = None

    def dump_to_primitive_dict(self):
        return self.model_dump(exclude={"username", "password"})

    def load_credentials_from_file(self, credentials_file_path: pathlib.Path):
        config = read_yaml_config_file(credentials_file_path)
        if config is None:
            raise ValueError(f"Credentials file '{credentials_file_path}' is empty.")
        try:
            self.username = get_config_value(config, f"{QUEUE_COMPONENT_NAME}.user")
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


class S3Credentials(BaseModel):
    access_key_id: NonEmptyStr
    secret_access_key: NonEmptyStr
    session_token: Optional[NonEmptyStr] = None


class AwsAuthentication(BaseModel):
    type: AwsAuthTypeStr
    profile: Optional[NonEmptyStr] = None
    credentials: Optional[S3Credentials] = None

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
    region_code: NonEmptyStr
    bucket: NonEmptyStr
    key_prefix: str
    aws_authentication: AwsAuthentication


class S3IngestionConfig(BaseModel):
    type: Literal[StorageType.S3.value] = StorageType.S3.value
    aws_authentication: AwsAuthentication


class FsStorage(BaseModel):
    type: Literal[StorageType.FS.value] = StorageType.FS.value
    directory: PathStr

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
    staging_directory: PathStr

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
    directory: PathStr = pathlib.Path("/")


class ArchiveFsStorage(FsStorage):
    directory: PathStr = CLP_DEFAULT_DATA_DIRECTORY_PATH / "archives"


class StreamFsStorage(FsStorage):
    directory: PathStr = CLP_DEFAULT_DATA_DIRECTORY_PATH / "streams"


class ArchiveS3Storage(S3Storage):
    staging_directory: PathStr = CLP_DEFAULT_DATA_DIRECTORY_PATH / "staged-archives"


class StreamS3Storage(S3Storage):
    staging_directory: PathStr = CLP_DEFAULT_DATA_DIRECTORY_PATH / "staged-streams"


def _get_directory_from_storage_config(
    storage_config: Union[FsStorage, S3Storage],
) -> pathlib.Path:
    storage_type = storage_config.type
    if StorageType.FS == storage_type:
        return storage_config.directory
    elif StorageType.S3 == storage_type:
        return storage_config.staging_directory
    else:
        raise NotImplementedError(f"storage.type {storage_type} is not supported")


def _set_directory_for_storage_config(
    storage_config: Union[FsStorage, S3Storage], directory
) -> None:
    storage_type = storage_config.type
    if StorageType.FS == storage_type:
        storage_config.directory = directory
    elif StorageType.S3 == storage_type:
        storage_config.staging_directory = directory
    else:
        raise NotImplementedError(f"storage.type {storage_type} is not supported")


class ArchiveOutput(BaseModel):
    storage: Union[ArchiveFsStorage, ArchiveS3Storage] = ArchiveFsStorage()
    target_archive_size: PositiveInt = 256 * 1024 * 1024  # 256 MB
    target_dictionaries_size: PositiveInt = 32 * 1024 * 1024  # 32 MB
    target_encoded_file_size: PositiveInt = 256 * 1024 * 1024  # 256 MB
    target_segment_size: PositiveInt = 256 * 1024 * 1024  # 256 MB
    compression_level: ZstdCompressionLevel = 3
    retention_period: Optional[PositiveInt] = None

    def set_directory(self, directory: pathlib.Path):
        _set_directory_for_storage_config(self.storage, directory)

    def get_directory(self) -> pathlib.Path:
        return _get_directory_from_storage_config(self.storage)


class StreamOutput(BaseModel):
    storage: Union[StreamFsStorage, StreamS3Storage] = StreamFsStorage()
    target_uncompressed_size: PositiveInt = 128 * 1024 * 1024

    def set_directory(self, directory: pathlib.Path):
        _set_directory_for_storage_config(self.storage, directory)

    def get_directory(self) -> pathlib.Path:
        return _get_directory_from_storage_config(self.storage)


class WebUi(BaseModel):
    host: DomainStr = "localhost"
    port: Port = 4000
    results_metadata_collection_name: NonEmptyStr = "results-metadata"
    rate_limit: PositiveInt = 1000


class SweepInterval(BaseModel):
    model_config = ConfigDict(extra="forbid")

    archive: PositiveInt = 60
    search_result: PositiveInt = 30


class GarbageCollector(BaseModel):
    logging_level: LoggingLevel = "INFO"
    sweep_interval: SweepInterval = SweepInterval()


class Presto(BaseModel):
    host: DomainStr
    port: Port


def _get_env_var(name: str) -> str:
    value = os.getenv(name)
    if value is None:
        raise ValueError(f"Missing environment variable: {name}")
    return value


class CLPConfig(BaseModel):
    container_image_ref: Optional[NonEmptyStr] = None

    logs_input: Union[FsIngestionConfig, S3IngestionConfig] = FsIngestionConfig()

    package: Package = Package()
    database: Database = Database()
    queue: Queue = Queue()
    redis: Redis = Redis()
    reducer: Reducer = Reducer()
    results_cache: ResultsCache = ResultsCache()
    compression_scheduler: CompressionScheduler = CompressionScheduler()
    query_scheduler: QueryScheduler = QueryScheduler()
    compression_worker: CompressionWorker = CompressionWorker()
    query_worker: QueryWorker = QueryWorker()
    webui: WebUi = WebUi()
    garbage_collector: GarbageCollector = GarbageCollector()
    credentials_file_path: PathStr = CLP_DEFAULT_CREDENTIALS_FILE_PATH

    presto: Optional[Presto] = None

    archive_output: ArchiveOutput = ArchiveOutput()
    stream_output: StreamOutput = StreamOutput()
    data_directory: PathStr = pathlib.Path("var") / "data"
    logs_directory: PathStr = pathlib.Path("var") / "log"
    aws_config_directory: Optional[pathlib.Path] = None

    _container_image_id_path: PathStr = PrivateAttr(default=CLP_PACKAGE_CONTAINER_IMAGE_ID_PATH)
    _version_file_path: PathStr = PrivateAttr(default=CLP_VERSION_FILE_PATH)

    @field_validator("aws_config_directory")
    @classmethod
    def expand_profile_user_home(cls, value: Optional[pathlib.Path]):
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
        self._container_image_id_path = make_config_path_absolute(
            clp_home, self._container_image_id_path
        )
        self._version_file_path = make_config_path_absolute(clp_home, self._version_file_path)

    def validate_logs_input_config(self):
        logs_input_type = self.logs_input.type
        if StorageType.FS == logs_input_type:
            # NOTE: This can't be a pydantic validator since input_logs_dir might be a
            # package-relative path that will only be resolved after pydantic validation
            input_logs_dir = self.logs_input.directory
            if not input_logs_dir.exists():
                raise ValueError(f"logs_input.directory '{input_logs_dir}' doesn't exist.")
            if not input_logs_dir.is_dir():
                raise ValueError(f"logs_input.directory '{input_logs_dir}' is not a directory.")
        if StorageType.S3 == logs_input_type and StorageEngine.CLP_S != self.package.storage_engine:
            raise ValueError(
                f"logs_input.type = 's3' is only supported with package.storage_engine"
                f" = '{StorageEngine.CLP_S}'"
            )

    def validate_archive_output_config(self):
        if (
            StorageType.S3 == self.archive_output.storage.type
            and StorageEngine.CLP_S != self.package.storage_engine
        ):
            raise ValueError(
                f"archive_output.storage.type = 's3' is only supported with package.storage_engine"
                f" = '{StorageEngine.CLP_S}'"
            )
        try:
            validate_path_could_be_dir(self.archive_output.get_directory())
        except ValueError as ex:
            raise ValueError(f"archive_output.storage's directory is invalid: {ex}")

    def validate_stream_output_config(self):
        if (
            StorageType.S3 == self.stream_output.storage.type
            and StorageEngine.CLP_S != self.package.storage_engine
        ):
            raise ValueError(
                f"stream_output.storage.type = 's3' is only supported with package.storage_engine"
                f" = '{StorageEngine.CLP_S}'"
            )
        try:
            validate_path_could_be_dir(self.stream_output.get_directory())
        except ValueError as ex:
            raise ValueError(f"stream_output.storage's directory is invalid: {ex}")

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

    def validate_aws_config_dir(self):
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
            if not self.aws_config_directory.exists():
                raise ValueError(
                    f"aws_config_directory does not exist: '{self.aws_config_directory}'"
                )
        if not profile_auth_used and self.aws_config_directory is not None:
            raise ValueError(
                "aws_config_directory should not be set when profile authentication is not used"
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

    def get_runnable_components(self) -> Set[str]:
        if QueryEngine.PRESTO == self.package.query_engine:
            return COMPRESSION_COMPONENTS | UI_COMPONENTS
        else:
            return ALL_COMPONENTS

    def dump_to_primitive_dict(self):
        custom_serialized_fields = {
            "database",
            "queue",
            "redis",
        }
        d = self.model_dump(exclude=custom_serialized_fields)
        for key in custom_serialized_fields:
            d[key] = getattr(self, key).dump_to_primitive_dict()

        # Turn paths into primitive strings
        if self.aws_config_directory is not None:
            d["aws_config_directory"] = str(self.aws_config_directory)
        else:
            d["aws_config_directory"] = None

        return d

    @model_validator(mode="after")
    def validate_presto_config(self):
        query_engine = self.package.query_engine
        presto = self.presto
        if query_engine == QueryEngine.PRESTO and presto is None:
            raise ValueError(
                f"`presto` config must be non-null when query_engine is `{query_engine}`"
            )
        return self


class WorkerConfig(BaseModel):
    package: Package = Package()
    archive_output: ArchiveOutput = ArchiveOutput()
    data_directory: pathlib.Path = CLPConfig().data_directory

    # Only needed by query workers.
    stream_output: StreamOutput = StreamOutput()
    stream_collection_name: str = ResultsCache().stream_collection_name


def get_components_for_target(target: str) -> Set[str]:
    if target in TARGET_TO_COMPONENTS:
        return TARGET_TO_COMPONENTS[target]
    elif target in ALL_COMPONENTS:
        return {target}
    else:
        return set()


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
