import pathlib
import typing

from pydantic import BaseModel, validator

from .core import get_config_value, make_config_path_absolute, read_yaml_config_file, validate_path_could_be_dir

# Constants
CLP_DEFAULT_CREDENTIALS_FILE_PATH = pathlib.Path('etc') / 'credentials.yml'
CLP_METADATA_TABLE_PREFIX = 'clp_'

# Component names
DB_COMPONENT_NAME = 'db'
RESULTS_CACHE_COMPONENT_NAME = 'results-cache'
COMPRESSION_JOB_HANDLER_COMPONENT_NAME = 'compression-job-handler'
COMPRESSION_QUEUE_COMPONENT_NAME = 'compression-queue'
SCHEDULER_COMPONENT_NAME = 'scheduler'
SEARCH_QUEUE_COMPONENT_NAME = 'search-queue'
SEARCH_SCHEDULER_COMPONENT_NAME = 'search-scheduler'
SEARCH_WORKER_COMPONENT_NAME = 'search-worker'
COMPRESSION_WORKER_COMPONENT_NAME = 'compression-worker'


class Database(BaseModel):
    type: str = 'mariadb'
    host: str = 'localhost'
    port: int = 3306
    name: str = 'clp-db'
    ssl_cert: typing.Optional[str] = None
    auto_commit: bool = False
    compress: bool = True

    username: typing.Optional[str] = None
    password: typing.Optional[str] = None

    @validator('type')
    def validate_database_type(cls, field):
        supported_database_types = ['mysql', 'mariadb']
        if field not in supported_database_types:
            raise ValueError(f"database.type must be one of the following {'|'.join(supported_database_types)}")
        return field

    @validator('name')
    def validate_database_name(cls, field):
        if '' == field:
            raise ValueError("database.name cannot be empty.")
        return field

    @validator('host')
    def validate_database_host(cls, field):
        if '' == field:
            raise ValueError("database.host cannot be empty.")
        return field

    def ensure_credentials_loaded(self):
        if self.username is None or self.password is None:
            raise ValueError("Credentials not loaded.")

    def get_mysql_connection_params(self, disable_localhost_socket_connection: bool = False):
        self.ensure_credentials_loaded()

        host = self.host
        if disable_localhost_socket_connection and 'localhost' == self.host:
            host = '127.0.0.1'

        # Currently, mysql's connection parameters are the same as mariadb
        connection_params = {
            'host': host,
            'port': self.port,
            'user': self.username,
            'password': self.password,
            'database': self.name,
            'compress': self.compress,
            'autocommit': self.auto_commit
        }
        if self.ssl_cert:
            connection_params['ssl_cert'] = self.ssl_cert
        return connection_params

    def get_clp_connection_params_and_type(self, disable_localhost_socket_connection: bool = False):
        self.ensure_credentials_loaded()

        host = self.host
        if disable_localhost_socket_connection and 'localhost' == self.host:
            host = '127.0.0.1'

        connection_params_and_type = {
            # NOTE: clp-core does not distinguish between mysql and mariadb
            'type': 'mysql',
            'host': host,
            'port': self.port,
            'username': self.username,
            'password': self.password,
            'name': self.name,
            'table_prefix': CLP_METADATA_TABLE_PREFIX,
            'compress': self.compress,
            'autocommit': self.auto_commit
        }
        if self.ssl_cert:
            connection_params_and_type['ssl_cert'] = self.ssl_cert
        return connection_params_and_type


class ResultsCache(BaseModel):
    host: str = 'localhost'
    port: int = 27017

    @validator('host')
    def validate_host(cls, field):
        if '' == field:
            raise ValueError(f'{RESULTS_CACHE_COMPONENT_NAME}.host cannot be empty.')
        return field

    def get_uri(self):
        # TODO Make database name configurable
        return f"mongodb://{self.host}:{self.port}/clp"


class Scheduler(BaseModel):
    jobs_poll_delay: int = 1  # seconds


class SearchScheduler(BaseModel):
    jobs_poll_delay: int = 1  # seconds


class Queue(BaseModel):
    host: str = 'localhost'
    port: int = 5672

    username: typing.Optional[str]
    password: typing.Optional[str]


class ArchiveOutput(BaseModel):
    directory: pathlib.Path = pathlib.Path('var') / 'data' / 'archives'
    target_archive_size: int = 256 * 1024 * 1024  # 256 MB
    target_dictionaries_size: int = 32 * 1024 * 1024  # 32 MB
    target_encoded_file_size: int = 256 * 1024 * 1024  # 256 MB
    target_segment_size: int = 256 * 1024 * 1024  # 256 MB

    @validator('target_archive_size')
    def validate_target_archive_size(cls, field):
        if field <= 0:
            raise ValueError('target_archive_size must be greater than 0')
        return field

    @validator('target_dictionaries_size')
    def validate_target_dictionaries_size(cls, field):
        if field <= 0:
            raise ValueError('target_dictionaries_size must be greater than 0')
        return field

    @validator('target_encoded_file_size')
    def validate_target_encoded_file_size(cls, field):
        if field <= 0:
            raise ValueError('target_encoded_file_size must be greater than 0')
        return field

    @validator('target_segment_size')
    def validate_target_segment_size(cls, field):
        if field <= 0:
            raise ValueError('target_segment_size must be greater than 0')
        return field

    def make_config_paths_absolute(self, clp_home: pathlib.Path):
        self.directory = make_config_path_absolute(clp_home, self.directory)

    def dump_to_primitive_dict(self):
        d = self.dict()
        # Turn directory (pathlib.Path) into a primitive string
        d['directory'] = str(d['directory'])
        return d


class CLPConfig(BaseModel):
    execution_container: str = 'ghcr.io/y-scope/clp/clp-execution-x86-ubuntu-focal:main'

    input_logs_directory: pathlib.Path = pathlib.Path('/')

    database: Database = Database()
    results_cache: ResultsCache = ResultsCache()
    scheduler: Scheduler = Scheduler()
    compression_queue: Queue = Queue(port=5672)
    search_queue: Queue = Queue(port=5673)
    search_scheduler: SearchScheduler = SearchScheduler()
    credentials_file_path: pathlib.Path = CLP_DEFAULT_CREDENTIALS_FILE_PATH

    archive_output: ArchiveOutput = ArchiveOutput()
    data_directory: pathlib.Path = pathlib.Path('var') / 'data'
    logs_directory: pathlib.Path = pathlib.Path('var') / 'log'

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
            self.database.username = get_config_value(config, 'db.user')
            self.database.password = get_config_value(config, 'db.password')
        except KeyError as ex:
            raise ValueError(f"Credentials file '{self.credentials_file_path}' does not contain key '{ex}'.")

    def load_compression_queue_credentials_from_file(self):
        self.__load_queue_credentials_from_file(self.credentials_file_path,
                                                COMPRESSION_QUEUE_COMPONENT_NAME,
                                                self.compression_queue)

    def load_search_queue_credentials_from_file(self):
        self.__load_queue_credentials_from_file(self.credentials_file_path,
                                                SEARCH_QUEUE_COMPONENT_NAME,
                                                self.search_queue)

    def dump_to_primitive_dict(self):
        d = self.dict()
        d['archive_output'] = self.archive_output.dump_to_primitive_dict()
        # Turn paths into primitive strings
        d['input_logs_directory'] = str(self.input_logs_directory)
        d['credentials_file_path'] = str(self.credentials_file_path)
        d['data_directory'] = str(self.data_directory)
        d['logs_directory'] = str(self.logs_directory)
        return d

    @staticmethod
    def __load_queue_credentials_from_file(credentials_file_path: pathlib.Path, component_name: str,
                                           queue_config: Queue):
        config = read_yaml_config_file(credentials_file_path)
        if config is None:
            raise ValueError(f"Credentials file '{credentials_file_path}' is empty.")
        try:
            queue_config.username = get_config_value(config, f"{component_name}.user")
            queue_config.password = get_config_value(config, f"{component_name}.password")
        except KeyError as ex:
            raise ValueError(f"Credentials file '{credentials_file_path}' does not contain key '{ex}'.")
