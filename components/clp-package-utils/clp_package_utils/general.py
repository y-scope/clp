import enum
import errno
import os
import pathlib
import secrets
import socket
import subprocess
import typing

import yaml

from clp_py_utils.clp_config import CLPConfig, CLP_DEFAULT_CREDENTIALS_FILE_PATH
from clp_py_utils.core import (
    get_config_value,
    make_config_path_absolute,
    read_yaml_config_file,
    validate_path_could_be_dir
)

# CONSTANTS
# Component names
DB_COMPONENT_NAME = 'db'
QUEUE_COMPONENT_NAME = 'queue'
SCHEDULER_COMPONENT_NAME = 'scheduler'
WORKER_COMPONENT_NAME = 'worker'

# Paths
CONTAINER_CLP_HOME = pathlib.Path('/') / 'opt' / 'clp'
CONTAINER_INPUT_LOGS_ROOT_DIR = pathlib.Path('/') / 'mnt' / 'logs'
CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH = pathlib.Path('etc') / 'clp-config.yml'

DOCKER_MOUNT_TYPE_STRINGS = [
    'bind'
]


class DockerMountType(enum.IntEnum):
    BIND = 0


class DockerMount:
    def __init__(self, type: DockerMountType, src: pathlib.Path, dst: pathlib.Path, is_read_only: bool = False):
        self.__type = type
        self.__src = src
        self.__dst = dst
        self.__is_read_only = is_read_only

    def __str__(self):
        mount_str = f"type={DOCKER_MOUNT_TYPE_STRINGS[self.__type]},src={self.__src},dst={self.__dst}"
        if self.__is_read_only:
            mount_str += ",readonly"
        return mount_str


class CLPDockerMounts:
    def __init__(self, clp_home: pathlib.Path, docker_clp_home: pathlib.Path):
        self.input_logs_dir: typing.Optional[DockerMount] = None
        self.clp_home: typing.Optional[DockerMount] = DockerMount(DockerMountType.BIND, clp_home, docker_clp_home)
        self.data_dir: typing.Optional[DockerMount] = None
        self.logs_dir: typing.Optional[DockerMount] = None
        self.archives_output_dir: typing.Optional[DockerMount] = None

def get_clp_home():
    # Determine CLP_HOME from an environment variable or this script's path
    clp_home = None
    if 'CLP_HOME' in os.environ:
        clp_home = pathlib.Path(os.environ['CLP_HOME'])
    else:
        for path in pathlib.Path(__file__).resolve().parents:
            if 'lib' == path.name:
                clp_home = path.parent
                break

    if clp_home is None:
        raise ValueError("CLP_HOME is not set and could not be determined automatically.")
    elif not clp_home.exists():
        raise ValueError("CLP_HOME set to nonexistent path.")

    return clp_home.resolve()

def check_dependencies():
    try:
        subprocess.run("command -v docker", shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    except subprocess.CalledProcessError:
        raise EnvironmentError("docker is not installed or available on the path")
    try:
        subprocess.run(['docker', 'ps'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    except subprocess.CalledProcessError:
        raise EnvironmentError("docker cannot run without superuser privileges (sudo).")


def container_exists(container_name):
    cmd = ['docker', 'ps', '-q', '-f', f'name={container_name}']
    proc = subprocess.run(cmd, stdout=subprocess.PIPE)
    for line in proc.stdout.decode('utf-8'):
        if line != "":
            return True

    return False


def validate_port(port_name: str, hostname: str, port: int):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((hostname, port))
        sock.close()
    except OSError as e:
        if e.errno == errno.EADDRINUSE:
            raise ValueError(f"{port_name} {hostname}:{port} is already in use. Please choose a different port.")
        else:
            raise ValueError(f"{port_name} {hostname}:{port} is invalid: {e.strerror}.")


def is_path_already_mounted(mounted_host_root: pathlib.Path, mounted_container_root: pathlib.Path,
                            host_path: pathlib.Path, container_path: pathlib.Path):
    try:
        host_path_relative_to_mounted_root = host_path.relative_to(mounted_host_root)
    except ValueError:
        return False

    try:
        container_path_relative_to_mounted_root = container_path.relative_to(mounted_container_root)
    except ValueError:
        return False

    return host_path_relative_to_mounted_root == container_path_relative_to_mounted_root


def generate_container_config(clp_config: CLPConfig, clp_home: pathlib.Path):
    """
    Copies the given config and sets up mounts mapping the relevant host paths into the container

    :param clp_config:
    :param clp_home:
    """
    container_clp_config = clp_config.copy(deep=True)

    docker_mounts = CLPDockerMounts(clp_home, CONTAINER_CLP_HOME)

    input_logs_dir = clp_config.input_logs_directory.resolve()
    container_clp_config.input_logs_directory = CONTAINER_INPUT_LOGS_ROOT_DIR / \
                                                input_logs_dir.relative_to(input_logs_dir.anchor)
    docker_mounts.input_logs_dir = DockerMount(DockerMountType.BIND, input_logs_dir,
                                               container_clp_config.input_logs_directory, True)

    container_clp_config.data_directory = CONTAINER_CLP_HOME / 'var' / 'data'
    if not is_path_already_mounted(clp_home, CONTAINER_CLP_HOME, clp_config.data_directory,
                                   container_clp_config.data_directory):
        docker_mounts.data_dir = DockerMount(DockerMountType.BIND, clp_config.data_directory,
                                             container_clp_config.data_directory)

    container_clp_config.logs_directory = CONTAINER_CLP_HOME / 'var' / 'log'
    if not is_path_already_mounted(clp_home, CONTAINER_CLP_HOME, clp_config.logs_directory,
                                   container_clp_config.logs_directory):
        docker_mounts.logs_dir = DockerMount(DockerMountType.BIND, clp_config.logs_directory,
                                             container_clp_config.logs_directory)

    container_clp_config.archive_output.directory = pathlib.Path('/') / 'mnt' / 'archive-output'
    if not is_path_already_mounted(clp_home, CONTAINER_CLP_HOME, clp_config.archive_output.directory,
                                   container_clp_config.archive_output.directory):
        docker_mounts.archives_output_dir = DockerMount(DockerMountType.BIND, clp_config.archive_output.directory,
                                                        container_clp_config.archive_output.directory)

    return container_clp_config, docker_mounts


def validate_config_key_existence(config, key):
    try:
        value = get_config_value(config, key)
    except KeyError:
        raise ValueError(f"{key} must be specified in CLP's configuration.")
    return value


def validate_and_load_config_file(config_file_path: pathlib.Path, default_config_file_path: pathlib.Path,
                                  clp_home: pathlib.Path):
    if config_file_path.exists():
        raw_clp_config = read_yaml_config_file(config_file_path)
        if raw_clp_config is None:
            clp_config = CLPConfig()
        else:
            clp_config = CLPConfig.parse_obj(raw_clp_config)
    else:
        if config_file_path != default_config_file_path:
            raise ValueError(f"Config file '{config_file_path}' does not exist.")

        clp_config = CLPConfig()

    clp_config.make_config_paths_absolute(clp_home)

    # Make data and logs directories node-specific
    hostname = socket.gethostname()
    clp_config.data_directory /= hostname
    clp_config.logs_directory /= hostname

    return clp_config


def generate_credentials_file(credentials_file_path: pathlib.Path):
    credentials = {
        DB_COMPONENT_NAME: {
            'user': 'clp-user',
            'password': secrets.token_urlsafe(8)
        },
        QUEUE_COMPONENT_NAME: {
            'user': 'clp-user',
            'password': secrets.token_urlsafe(8)
        },
    }

    with open(credentials_file_path, 'w') as f:
        yaml.safe_dump(credentials, f)


def validate_credentials_file_path(clp_config: CLPConfig, clp_home: pathlib.Path, generate_default_file: bool):
    credentials_file_path = clp_config.credentials_file_path
    if not credentials_file_path.exists():
        if make_config_path_absolute(clp_home, CLP_DEFAULT_CREDENTIALS_FILE_PATH) == credentials_file_path \
                and generate_default_file:
            generate_credentials_file(credentials_file_path)
        else:
            raise ValueError(f"Credentials file path '{credentials_file_path}' does not exist.")
    elif not credentials_file_path.is_file():
        raise ValueError(f"Credentials file path '{credentials_file_path}' is not a file.")


def validate_and_load_db_credentials_file(clp_config: CLPConfig, clp_home: pathlib.Path, generate_default_file: bool):
    validate_credentials_file_path(clp_config, clp_home, generate_default_file)
    clp_config.load_database_credentials_from_file()


def validate_and_load_queue_credentials_file(clp_config: CLPConfig, clp_home: pathlib.Path,
                                             generate_default_file: bool):
    validate_credentials_file_path(clp_config, clp_home, generate_default_file)
    clp_config.load_queue_credentials_from_file()


def validate_db_config(clp_config: CLPConfig, data_dir: pathlib.Path, logs_dir: pathlib.Path):
    try:
        validate_path_could_be_dir(data_dir)
    except ValueError as ex:
        raise ValueError(f"database data directory is invalid: {ex}")

    try:
        validate_path_could_be_dir(logs_dir)
    except ValueError as ex:
        raise ValueError(f"database logs directory is invalid: {ex}")

    validate_port("database.port", clp_config.database.host, clp_config.database.port)


def validate_queue_config(clp_config: CLPConfig, logs_dir: pathlib.Path):
    try:
        validate_path_could_be_dir(logs_dir)
    except ValueError as ex:
        raise ValueError(f"queue logs directory is invalid: {ex}")

    validate_port("queue.port", clp_config.queue.host, clp_config.queue.port)


def validate_worker_config(clp_config: CLPConfig):
    clp_config.validate_input_logs_dir()
    clp_config.validate_archive_output_dir()
