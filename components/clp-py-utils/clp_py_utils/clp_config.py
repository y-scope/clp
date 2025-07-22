import pathlib
from enum import auto
from typing import Literal, Optional, Union

from dotenv import dotenv_values
from pydantic import BaseModel, PrivateAttr, root_validator, validator
from strenum import KebabCaseStrEnum, LowercaseStrEnum

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
REDUCER_COMPONENT_NAME = "reducer"
RESULTS_CACHE_COMPONENT_NAME = "results_cache"
COMPRESSION_SCHEDULER_COMPONENT_NAME = "compression_scheduler"
QUERY_SCHEDULER_COMPONENT_NAME = "query_scheduler"
COMPRESSION_WORKER_COMPONENT_NAME = "compression_worker"
QUERY_WORKER_COMPONENT_NAME = "query_worker"
WEBUI_COMPONENT_NAME = "webui"

# Target names
ALL_TARGET_NAME = ""
CONTROLLER_TARGET_NAME = "controller"

QUERY_JOBS_TABLE_NAME = "query_jobs"
QUERY_TASKS_TABLE_NAME = "query_tasks"
COMPRESSION_JOBS_TABLE_NAME = "compression_jobs"
COMPRESSION_TASKS_TABLE_NAME = "compression_tasks"

OS_RELEASE_FILE_PATH = pathlib.Path("etc") / "os-release"

CLP_DEFAULT_CREDENTIALS_FILE_PATH = pathlib.Path("etc") / "credentials.yml"
CLP_DEFAULT_DATA_DIRECTORY_PATH = pathlib.Path("var") / "data"
CLP_DEFAULT_DATASET_NAME = "default"
CLP_METADATA_TABLE_PREFIX = "clp_"


class StorageEngine(KebabCaseStrEnum):
    CLP = auto()
    CLP_S = auto()


class StorageType(LowercaseStrEnum):
    FS = auto()
    S3 = auto()


class AwsAuthType(LowercaseStrEnum):
    credentials = auto()
    profile = auto()
    env_vars = auto()
    ec2 = auto()


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
    ssl_cert: Optional[str] = None
    auto_commit: bool = False
    compress: bool = True

    username: Optional[str] = None
    password: Optional[str] = None

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


def _validate_host(cls, field):
    if "" == field:
        raise ValueError(f"{cls.__name__}.host cannot be empty.")


def _validate_port(cls, field):
    min_valid_port = 0
    max_valid_port = 2**16 - 1
    if min_valid_port > field or max_valid_port < field:
        raise ValueError(
            f"{cls.__name__}.port is not within valid range " f"{min_valid_port}-{max_valid_port}."
        )


class CompressionScheduler(BaseModel):
    jobs_poll_delay: float = 0.1  # seconds
    logging_level: str = "INFO"

    @validator("logging_level")
    def validate_logging_level(cls, field):
        _validate_logging_level(cls, field)
        return field


class QueryScheduler(BaseModel):
    host = "localhost"
    port = 7000
    jobs_poll_delay: float = 0.1  # seconds
    num_archives_to_search_per_sub_job: int = 16
    logging_level: str = "INFO"

    @validator("logging_level")
    def validate_logging_level(cls, field):
        _validate_logging_level(cls, field)
        return field

    @validator("host")
    def validate_host(cls, field):
        if "" == field:
            raise ValueError(f"Cannot be empty.")
        return field

    @validator("port")
    def validate_port(cls, field):
        if not field > 0:
            raise ValueError(f"{field} is not greater than zero")
        return field


class CompressionWorker(BaseModel):
    logging_level: str = "INFO"

    @validator("logging_level")
    def validate_logging_level(cls, field):
        _validate_logging_level(cls, field)
        return field


class QueryWorker(BaseModel):
    logging_level: str = "INFO"

    @validator("logging_level")
    def validate_logging_level(cls, field):
        _validate_logging_level(cls, field)
        return field


class Redis(BaseModel):
    host: str = "localhost"
    port: int = 6379
    query_backend_database: int = 0
    compression_backend_database: int = 1
    # redis can perform authentication without a username
    password: Optional[str]

    @validator("host")
    def validate_host(cls, field):
        if "" == field:
            raise ValueError(f"{REDIS_COMPONENT_NAME}.host cannot be empty.")
        return field


class Reducer(BaseModel):
    host: str = "localhost"
    base_port: int = 14009
    logging_level: str = "INFO"
    upsert_interval: int = 100  # milliseconds

    @validator("host")
    def validate_host(cls, field):
        if "" == field:
            raise ValueError(f"{field} cannot be empty")
        return field

    @validator("logging_level")
    def validate_logging_level(cls, field):
        _validate_logging_level(cls, field)
        return field

    @validator("base_port")
    def validate_base_port(cls, field):
        if not field > 0:
            raise ValueError(f"{field} is not greater than zero")
        return field

    @validator("upsert_interval")
    def validate_upsert_interval(cls, field):
        if not field > 0:
            raise ValueError(f"{field} is not greater than zero")
        return field


class ResultsCache(BaseModel):
    host: str = "localhost"
    port: int = 27017
    db_name: str = "clp-query-results"
    stream_collection_name: str = "stream-files"

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

    @validator("stream_collection_name")
    def validate_stream_collection_name(cls, field):
        if "" == field:
            raise ValueError(
                f"{RESULTS_CACHE_COMPONENT_NAME}.stream_collection_name cannot be empty."
            )
        return field

    def get_uri(self):
        return f"mongodb://{self.host}:{self.port}/{self.db_name}"


class Queue(BaseModel):
    host: str = "localhost"
    port: int = 5672

    username: Optional[str]
    password: Optional[str]


class S3Credentials(BaseModel):
    access_key_id: str
    secret_access_key: str
    session_token: Optional[str]

    @validator("access_key_id")
    def validate_access_key_id(cls, field):
        if "" == field:
            raise ValueError("access_key_id cannot be empty")
        return field

    @validator("secret_access_key")
    def validate_secret_access_key(cls, field):
        if "" == field:
            raise ValueError("secret_access_key cannot be empty")
        return field


class AwsAuthentication(BaseModel):
    type: Literal[
        AwsAuthType.credentials.value,
        AwsAuthType.profile.value,
        AwsAuthType.env_vars.value,
        AwsAuthType.ec2.value,
    ]
    profile: Optional[str] = None
    credentials: Optional[S3Credentials] = None

    @root_validator(pre=True)
    def validate_authentication(cls, values):
        auth_type = values.get("type")
        profile = values.get("profile")
        credentials = values.get("credentials")

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
        return values


class S3Config(BaseModel):
    region_code: str
    bucket: str
    key_prefix: str
    aws_authentication: AwsAuthentication

    @validator("region_code")
    def validate_region_code(cls, field):
        if "" == field:
            raise ValueError("region_code cannot be empty")
        return field

    @validator("bucket")
    def validate_bucket(cls, field):
        if "" == field:
            raise ValueError("bucket cannot be empty")
        return field


class S3IngestionConfig(BaseModel):
    type: Literal[StorageType.S3.value] = StorageType.S3.value
    aws_authentication: AwsAuthentication

    def dump_to_primitive_dict(self):
        return self.dict()


class FsStorage(BaseModel):
    type: Literal[StorageType.FS.value] = StorageType.FS.value
    directory: pathlib.Path

    @validator("directory")
    def validate_directory(cls, field):
        if "" == field:
            raise ValueError("directory cannot be empty")
        return field

    def make_config_paths_absolute(self, clp_home: pathlib.Path):
        self.directory = make_config_path_absolute(clp_home, self.directory)

    def dump_to_primitive_dict(self):
        d = self.dict()
        d["directory"] = str(d["directory"])
        return d


class S3Storage(BaseModel):
    type: Literal[StorageType.S3.value] = StorageType.S3.value
    s3_config: S3Config
    staging_directory: pathlib.Path

    @validator("staging_directory")
    def validate_staging_directory(cls, field):
        if "" == field:
            raise ValueError("staging_directory cannot be empty")
        return field

    @root_validator
    def validate_key_prefix(cls, values):
        s3_config = values.get("s3_config")
        if not hasattr(s3_config, "key_prefix"):
            raise ValueError("s3_config must have field key_prefix")
        key_prefix = s3_config.key_prefix
        if "" == key_prefix:
            raise ValueError("s3_config.key_prefix cannot be empty")
        if not key_prefix.endswith("/"):
            raise ValueError('s3_config.key_prefix must end with "/"')
        return values

    def make_config_paths_absolute(self, clp_home: pathlib.Path):
        self.staging_directory = make_config_path_absolute(clp_home, self.staging_directory)

    def dump_to_primitive_dict(self):
        d = self.dict()
        d["staging_directory"] = str(d["staging_directory"])
        return d


class FsIngestionConfig(FsStorage):
    directory: pathlib.Path = pathlib.Path("/")


class ArchiveFsStorage(FsStorage):
    directory: pathlib.Path = CLP_DEFAULT_DATA_DIRECTORY_PATH / "archives"


class StreamFsStorage(FsStorage):
    directory: pathlib.Path = CLP_DEFAULT_DATA_DIRECTORY_PATH / "streams"


class ArchiveS3Storage(S3Storage):
    staging_directory: pathlib.Path = CLP_DEFAULT_DATA_DIRECTORY_PATH / "staged-archives"


class StreamS3Storage(S3Storage):
    staging_directory: pathlib.Path = CLP_DEFAULT_DATA_DIRECTORY_PATH / "staged-streams"


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
    target_archive_size: int = 256 * 1024 * 1024  # 256 MB
    target_dictionaries_size: int = 32 * 1024 * 1024  # 32 MB
    target_encoded_file_size: int = 256 * 1024 * 1024  # 256 MB
    target_segment_size: int = 256 * 1024 * 1024  # 256 MB
    compression_level: int = 3

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

    @validator("compression_level")
    def validate_compression_level(cls, field):
        if field < 1 or 19 < field:
            raise ValueError("compression_level must be a value from 1 to 19")
        return field

    def set_directory(self, directory: pathlib.Path):
        _set_directory_for_storage_config(self.storage, directory)

    def get_directory(self) -> pathlib.Path:
        return _get_directory_from_storage_config(self.storage)

    def dump_to_primitive_dict(self):
        d = self.dict()
        d["storage"] = self.storage.dump_to_primitive_dict()
        return d


class StreamOutput(BaseModel):
    storage: Union[StreamFsStorage, StreamS3Storage] = StreamFsStorage()
    target_uncompressed_size: int = 128 * 1024 * 1024

    @validator("target_uncompressed_size")
    def validate_target_uncompressed_size(cls, field):
        if field <= 0:
            raise ValueError("target_uncompressed_size must be greater than 0")
        return field

    def set_directory(self, directory: pathlib.Path):
        _set_directory_for_storage_config(self.storage, directory)

    def get_directory(self) -> pathlib.Path:
        return _get_directory_from_storage_config(self.storage)

    def dump_to_primitive_dict(self):
        d = self.dict()
        d["storage"] = self.storage.dump_to_primitive_dict()
        return d


class WebUi(BaseModel):
    host: str = "localhost"
    port: int = 4000
    results_metadata_collection_name: str = "results-metadata"

    @validator("host")
    def validate_host(cls, field):
        _validate_host(cls, field)
        return field

    @validator("port")
    def validate_port(cls, field):
        _validate_port(cls, field)
        return field

    @validator("results_metadata_collection_name")
    def validate_results_metadata_collection_name(cls, field):
        if "" == field:
            raise ValueError(
                f"{WEBUI_COMPONENT_NAME}.results_metadata_collection_name cannot be empty."
            )
        return field


class CLPConfig(BaseModel):
    execution_container: Optional[str] = None

    logs_input: Union[FsIngestionConfig, S3IngestionConfig] = FsIngestionConfig()

    package: Package = Package()
    database: Database = Database()
    queue: Queue = Queue()
    redis: Redis = Redis()
    reducer: Reducer() = Reducer()
    results_cache: ResultsCache = ResultsCache()
    compression_scheduler: CompressionScheduler = CompressionScheduler()
    query_scheduler: QueryScheduler = QueryScheduler()
    compression_worker: CompressionWorker = CompressionWorker()
    query_worker: QueryWorker = QueryWorker()
    webui: WebUi = WebUi()
    credentials_file_path: pathlib.Path = CLP_DEFAULT_CREDENTIALS_FILE_PATH

    archive_output: ArchiveOutput = ArchiveOutput()
    stream_output: StreamOutput = StreamOutput()
    data_directory: pathlib.Path = pathlib.Path("var") / "data"
    logs_directory: pathlib.Path = pathlib.Path("var") / "log"
    aws_config_directory: Optional[pathlib.Path] = None

    _os_release_file_path: pathlib.Path = PrivateAttr(default=OS_RELEASE_FILE_PATH)

    def make_config_paths_absolute(self, clp_home: pathlib.Path):
        if StorageType.FS == self.logs_input.type:
            self.logs_input.make_config_paths_absolute(clp_home)
        self.credentials_file_path = make_config_path_absolute(clp_home, self.credentials_file_path)
        self.archive_output.storage.make_config_paths_absolute(clp_home)
        self.stream_output.storage.make_config_paths_absolute(clp_home)
        self.data_directory = make_config_path_absolute(clp_home, self.data_directory)
        self.logs_directory = make_config_path_absolute(clp_home, self.logs_directory)
        self._os_release_file_path = make_config_path_absolute(clp_home, self._os_release_file_path)

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
            if AwsAuthType.profile.value == auth.type:
                profile_auth_used = True
                break

        if profile_auth_used:
            if self.aws_config_directory is None:
                raise ValueError(
                    "aws_config_directory must be set when using profile authentication"
                )
            if not self.aws_config_directory.exists():
                raise ValueError("aws_config_directory does not exist")
        if not profile_auth_used and self.aws_config_directory is not None:
            raise ValueError(
                "aws_config_directory should not be set when profile authentication is not used"
            )

    def load_execution_container_name(self):
        if self.execution_container is not None:
            # Accept configured value for debug purposes
            return

        os_release = dotenv_values(self._os_release_file_path)
        if "ubuntu" == os_release["ID"]:
            self.execution_container = (
                f"clp-execution-x86-{os_release['ID']}-{os_release['VERSION_CODENAME']}:main"
            )
        else:
            raise NotImplementedError(
                f"Unsupported OS {os_release['ID']} in {OS_RELEASE_FILE_PATH}"
            )

        self.execution_container = "ghcr.io/y-scope/clp/" + self.execution_container

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
        d["logs_input"] = self.logs_input.dump_to_primitive_dict()
        d["archive_output"] = self.archive_output.dump_to_primitive_dict()
        d["stream_output"] = self.stream_output.dump_to_primitive_dict()
        # Turn paths into primitive strings
        d["credentials_file_path"] = str(self.credentials_file_path)
        d["data_directory"] = str(self.data_directory)
        d["logs_directory"] = str(self.logs_directory)
        if self.aws_config_directory is not None:
            d["aws_config_directory"] = str(self.aws_config_directory)
        else:
            d["aws_config_directory"] = None
        return d


class WorkerConfig(BaseModel):
    package: Package = Package()
    archive_output: ArchiveOutput = ArchiveOutput()
    data_directory: pathlib.Path = CLPConfig().data_directory

    # Only needed by query workers.
    stream_output: StreamOutput = StreamOutput()
    stream_collection_name: str = ResultsCache().stream_collection_name

    def dump_to_primitive_dict(self):
        d = self.dict()
        d["archive_output"] = self.archive_output.dump_to_primitive_dict()

        # Turn paths into primitive strings
        d["data_directory"] = str(self.data_directory)
        d["stream_output"] = self.stream_output.dump_to_primitive_dict()

        return d
