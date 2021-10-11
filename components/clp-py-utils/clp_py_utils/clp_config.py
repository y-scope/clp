import typing

from pydantic import BaseModel, validator

from clp_py_utils.pretty_size import pretty_size


class Database(BaseModel):
    type: str
    host: str
    port: int
    username: str
    password: str
    name: str
    ssl_cert: typing.Optional[str] = None
    auto_commit: bool = False
    compress: bool = True

    @validator('type')
    def validate_database_type(cls, field):
        supported_database_type = ['mysql', 'mariadb', 'bundled']
        if field not in supported_database_type:
            raise ValueError(f'must be one of the following {"|".join(supported_database_type)}')
        return field

    def get_mysql_connection_params(self):
        # Currently, mysql's connector parameter is the same as mariadb
        connection_params = {
            'host': self.host,
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

    def get_mariadb_connection_params(self):
        # Currently, mysql's connector parameter is the same as mysql
        connection_params = {
            'host': self.host,
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

    def get_clp_connection_params_and_type(self):
        connection_params_and_type = {
            'type': 'mysql',  # hard code this as mysql as CLP only support "mysql" for global database
            'host': self.host,
            'port': self.port,
            'username': self.username,
            'password': self.password,
            'name': self.name,
            'table_prefix': 'clp_',
            'compress': self.compress,
            'autocommit': self.auto_commit
        }
        if self.ssl_cert:
            connection_params_and_type['ssl_cert'] = self.ssl_cert
        return connection_params_and_type


class Scheduler(BaseModel):
    host: str
    username: str
    password: str
    jobs_poll_delay: int


class ArchiveOutput(BaseModel):
    type: str  # Support only 'fs' type for now
    directory: str
    storage_is_node_specific: bool = False
    target_archive_size: int
    target_dictionaries_size: int
    target_encoded_file_size: int
    target_segment_size: int

    @validator('type')
    def validate_type(cls, field):
        if 'fs' != field:
            raise ValueError('only fs type is supported in the opensource distribution')
        return field

    @validator('target_archive_size')
    def validate_target_archive_size(cls, field):
        if field <= 0:
            raise ValueError('target_archive_size parameter must be greater than 0')
        return field

    @validator('target_dictionaries_size')
    def validate_target_dictionaries_size(cls, field):
        if field <= 0:
            raise ValueError('target_dictionaries_size parameter must be greater than 0')
        return field

    @validator('target_encoded_file_size')
    def validate_target_encoded_file_size(cls, field):
        if field <= 0:
            raise ValueError('target_encoded_file_size parameter must be greater than 0')
        return field

    @validator('target_segment_size')
    def validate_target_segment_size(cls, field):
        if field <= 0:
            raise ValueError('target_segment_size parameter must be greater than 0')
        return field


class CLPConfig(BaseModel):
    input_logs_dfs_path: str
    database: Database
    scheduler: Scheduler
    archive_output: ArchiveOutput
    data_directory: str
    logs_directory: str

    def generate_config_file_content_with_comments(self):
        file_content = [
            f'# A path containing any logs you which to compress. Must be reachable by all workers.',
            f'# - This path will be exposed inside the docker container.',
            f'# - This path should not be any path that exists in the container image (an Ubuntu image) (e.g., /var/log).',
            f'# - Limitations: Docker follow symlink outside context, therefore, we recommend avoiding symbolic links',
            f'input_logs_dfs_path: {self.input_logs_dfs_path}',
            f'',
            f'database:',
            f'  type: {self.database.type}',
            f'  host: {self.database.host}',
            f'  port: {self.database.port}',
            f'  username: {self.database.username}',
            f'  password: {self.database.password}',
            f'  name: {self.database.name}',
            f'',
            f'scheduler:',
            f'  host: {self.scheduler.host}',
            f'  username: {self.scheduler.username}',
            f'  password: {self.scheduler.password}',
            f'  jobs_poll_delay: {self.scheduler.jobs_poll_delay}   # Seconds',
            f'',
            f'# Where archives should be output to',
            f'# Note: Only one output type may be specified',
            f'archive_output:',
            f'  type: {self.archive_output.type}',
            f'  directory: "{self.archive_output.directory}"',
            f'',
            f'  storage_is_node_specific: {self.archive_output.storage_is_node_specific}',
            f'',
            f'  # How much data CLP should try to compress into each archive',
            f'  target_archive_size: {self.archive_output.target_archive_size}   # {pretty_size(self.archive_output.target_archive_size)}',
            f'',
            f'  # How large the dictionaries should be allowed to get before the archive is closed and a new one is created',
            f'  target_dictionaries_size: {self.archive_output.target_dictionaries_size}   # {pretty_size(self.archive_output.target_dictionaries_size)}',
            f'',
            f'  # How large each encoded file should be before being split into a new encoded file',
            f'  target_encoded_file_size: {self.archive_output.target_encoded_file_size}   # {pretty_size(self.archive_output.target_encoded_file_size)}',
            f'',
            f'  # How much data CLP should try to fit into each segment within an archive',
            f'  target_segment_size: {self.archive_output.target_segment_size}   # {pretty_size(self.archive_output.target_segment_size)}',
            f'',
            f'# Location where other data is stored',
            f'data_directory: "{self.data_directory}"',
            f'',
            f'# Location where logs are stored',
            f'logs_directory: "{self.logs_directory}"',
            f'',
        ]
        return '\n'.join(file_content)
