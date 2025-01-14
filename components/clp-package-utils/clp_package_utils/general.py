import enum
import errno
import os
import pathlib
import secrets
import socket
import subprocess
import typing
import uuid
from enum import auto
from typing import List, Optional, Tuple

import yaml
from clp_py_utils.clp_config import (
    CLP_DEFAULT_CREDENTIALS_FILE_PATH,
    CLPConfig,
    DB_COMPONENT_NAME,
    LOG_VIEWER_WEBUI_COMPONENT_NAME,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    WEBUI_COMPONENT_NAME,
    WorkerConfig,
)
from clp_py_utils.core import (
    get_config_value,
    make_config_path_absolute,
    read_yaml_config_file,
    validate_path_could_be_dir,
)
from strenum import KebabCaseStrEnum

# CONSTANTS
EXTRACT_FILE_CMD = "x"
EXTRACT_IR_CMD = "i"
EXTRACT_JSON_CMD = "j"

# Paths
CONTAINER_CLP_HOME = pathlib.Path("/") / "opt" / "clp"
CONTAINER_INPUT_LOGS_ROOT_DIR = pathlib.Path("/") / "mnt" / "logs"
CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH = pathlib.Path("etc") / "clp-config.yml"

DOCKER_MOUNT_TYPE_STRINGS = ["bind"]


class DockerMountType(enum.IntEnum):
    BIND = 0


class JobType(KebabCaseStrEnum):
    COMPRESSION = auto()
    FILE_EXTRACTION = auto()
    IR_EXTRACTION = auto()
    SEARCH = auto()


class DockerMount:
    def __init__(
        self,
        type: DockerMountType,
        src: pathlib.Path,
        dst: pathlib.Path,
        is_read_only: bool = False,
    ):
        self.__type = type
        self.__src = src
        self.__dst = dst
        self.__is_read_only = is_read_only

    def __str__(self):
        mount_str = (
            f"type={DOCKER_MOUNT_TYPE_STRINGS[self.__type]},src={self.__src},dst={self.__dst}"
        )
        if self.__is_read_only:
            mount_str += ",readonly"
        return mount_str


class CLPDockerMounts:
    def __init__(self, clp_home: pathlib.Path, docker_clp_home: pathlib.Path):
        self.input_logs_dir: typing.Optional[DockerMount] = None
        self.clp_home: typing.Optional[DockerMount] = DockerMount(
            DockerMountType.BIND, clp_home, docker_clp_home
        )
        self.data_dir: typing.Optional[DockerMount] = None
        self.logs_dir: typing.Optional[DockerMount] = None
        self.archives_output_dir: typing.Optional[DockerMount] = None
        self.stream_output_dir: typing.Optional[DockerMount] = None


def get_clp_home():
    # Determine CLP_HOME from an environment variable or this script's path
    clp_home = None
    if "CLP_HOME" in os.environ:
        clp_home = pathlib.Path(os.environ["CLP_HOME"])
    else:
        for path in pathlib.Path(__file__).resolve().parents:
            if "lib" == path.name:
                clp_home = path.parent
                break

    if clp_home is None:
        raise ValueError("CLP_HOME is not set and could not be determined automatically.")
    elif not clp_home.exists():
        raise ValueError("CLP_HOME set to nonexistent path.")

    return clp_home.resolve()


def generate_container_name(job_type: str) -> str:
    """
    :param job_type:
    :return: A unique container name for the given job type.
    """
    return f"clp-{job_type}-{str(uuid.uuid4())[-4:]}"


def check_dependencies():
    try:
        subprocess.run(
            "command -v docker",
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=True,
        )
    except subprocess.CalledProcessError:
        raise EnvironmentError("docker is not installed or available on the path")
    try:
        subprocess.run(
            ["docker", "ps"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True
        )
    except subprocess.CalledProcessError:
        raise EnvironmentError("docker cannot run without superuser privileges (sudo).")


def is_container_running(container_name):
    # fmt: off
    cmd = [
        "docker", "ps",
        # Only return container IDs
        "--quiet",
        "--filter", f"name={container_name}"
    ]
    # fmt: on
    proc = subprocess.run(cmd, stdout=subprocess.PIPE)
    if proc.stdout.decode("utf-8"):
        return True

    return False


def is_container_exited(container_name):
    # fmt: off
    cmd = [
        "docker", "ps",
        # Only return container IDs
        "--quiet",
        "--filter", f"name={container_name}",
        "--filter", "status=exited"
    ]
    # fmt: on
    proc = subprocess.run(cmd, stdout=subprocess.PIPE)
    if proc.stdout.decode("utf-8"):
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
            raise ValueError(
                f"{port_name} {hostname}:{port} is already in use. Please choose a different port."
            )
        else:
            raise ValueError(f"{port_name} {hostname}:{port} is invalid: {e.strerror}.")


def is_path_already_mounted(
    mounted_host_root: pathlib.Path,
    mounted_container_root: pathlib.Path,
    host_path: pathlib.Path,
    container_path: pathlib.Path,
):
    try:
        host_path_relative_to_mounted_root = host_path.relative_to(mounted_host_root)
    except ValueError:
        return False

    try:
        container_path_relative_to_mounted_root = container_path.relative_to(mounted_container_root)
    except ValueError:
        return False

    return host_path_relative_to_mounted_root == container_path_relative_to_mounted_root


def generate_container_config(
    clp_config: CLPConfig, clp_home: pathlib.Path
) -> Tuple[CLPConfig, CLPDockerMounts]:
    """
    Copies the given config and sets up mounts mapping the relevant host paths into the container

    :param clp_config:
    :param clp_home:
    :return: The container config and the mounts.
    """
    container_clp_config = clp_config.copy(deep=True)

    docker_mounts = CLPDockerMounts(clp_home, CONTAINER_CLP_HOME)

    input_logs_dir = clp_config.input_logs_directory.resolve()
    container_clp_config.input_logs_directory = (
        CONTAINER_INPUT_LOGS_ROOT_DIR / input_logs_dir.relative_to(input_logs_dir.anchor)
    )
    docker_mounts.input_logs_dir = DockerMount(
        DockerMountType.BIND, input_logs_dir, container_clp_config.input_logs_directory, True
    )

    container_clp_config.data_directory = CONTAINER_CLP_HOME / "var" / "data"
    if not is_path_already_mounted(
        clp_home, CONTAINER_CLP_HOME, clp_config.data_directory, container_clp_config.data_directory
    ):
        docker_mounts.data_dir = DockerMount(
            DockerMountType.BIND, clp_config.data_directory, container_clp_config.data_directory
        )

    container_clp_config.logs_directory = CONTAINER_CLP_HOME / "var" / "log"
    if not is_path_already_mounted(
        clp_home, CONTAINER_CLP_HOME, clp_config.logs_directory, container_clp_config.logs_directory
    ):
        docker_mounts.logs_dir = DockerMount(
            DockerMountType.BIND, clp_config.logs_directory, container_clp_config.logs_directory
        )

    container_clp_config.archive_output.set_directory(pathlib.Path("/") / "mnt" / "archive-output")
    if not is_path_already_mounted(
        clp_home,
        CONTAINER_CLP_HOME,
        clp_config.archive_output.get_directory(),
        container_clp_config.archive_output.get_directory(),
    ):
        docker_mounts.archives_output_dir = DockerMount(
            DockerMountType.BIND,
            clp_config.archive_output.get_directory(),
            container_clp_config.archive_output.get_directory(),
        )

    container_clp_config.stream_output.set_directory(pathlib.Path("/") / "mnt" / "stream-output")
    if not is_path_already_mounted(
        clp_home,
        CONTAINER_CLP_HOME,
        clp_config.stream_output.get_directory(),
        container_clp_config.stream_output.get_directory(),
    ):
        docker_mounts.stream_output_dir = DockerMount(
            DockerMountType.BIND,
            clp_config.stream_output.get_directory(),
            container_clp_config.stream_output.get_directory(),
        )

    return container_clp_config, docker_mounts


def generate_worker_config(clp_config: CLPConfig) -> WorkerConfig:
    worker_config = WorkerConfig()
    worker_config.package = clp_config.package.copy(deep=True)
    worker_config.archive_output = clp_config.archive_output.copy(deep=True)
    worker_config.data_directory = clp_config.data_directory

    worker_config.stream_output = clp_config.stream_output
    worker_config.stream_collection_name = clp_config.results_cache.stream_collection_name

    return worker_config


def dump_container_config(
    container_clp_config: CLPConfig, clp_config: CLPConfig, container_name: str
) -> Tuple[pathlib.Path, pathlib.Path]:
    """
    Writes the given config to the logs directory so that it's accessible in the container.
    :param container_clp_config: The config to write.
    :param clp_config: The corresponding config on the host (used to determine the logs directory).
    :param container_name:
    :return: The path to the config file in the container and on the host.
    """
    container_config_filename = f".{container_name}-config.yml"
    config_file_path_on_host = clp_config.logs_directory / container_config_filename
    config_file_path_on_container = container_clp_config.logs_directory / container_config_filename
    with open(config_file_path_on_host, "w") as f:
        yaml.safe_dump(container_clp_config.dump_to_primitive_dict(), f)

    return config_file_path_on_container, config_file_path_on_host


def generate_container_start_cmd(
    container_name: str, container_mounts: List[Optional[DockerMount]], container_image: str
) -> List[str]:
    """
    Generates the command to start a container with the given mounts and name.
    :param container_name:
    :param container_mounts:
    :param container_image:
    :return: The command.
    """
    clp_site_packages_dir = CONTAINER_CLP_HOME / "lib" / "python3" / "site-packages"
    # fmt: off
    container_start_cmd = [
        "docker", "run",
        "-i",
        "--rm",
        "--network", "host",
        "-w", str(CONTAINER_CLP_HOME),
        "-e", f"PYTHONPATH={clp_site_packages_dir}",
        "-u", f"{os.getuid()}:{os.getgid()}",
        "--name", container_name,
        "--log-driver", "local"
    ]
    for mount in container_mounts:
        if mount:
            container_start_cmd.append("--mount")
            container_start_cmd.append(str(mount))
    container_start_cmd.append(container_image)

    return container_start_cmd


def validate_config_key_existence(config, key):
    try:
        value = get_config_value(config, key)
    except KeyError:
        raise ValueError(f"{key} must be specified in CLP's configuration.")
    return value


def load_config_file(
    config_file_path: pathlib.Path, default_config_file_path: pathlib.Path, clp_home: pathlib.Path
):
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
    clp_config.load_execution_container_name()

    # Make data and logs directories node-specific
    hostname = socket.gethostname()
    clp_config.data_directory /= hostname
    clp_config.logs_directory /= hostname

    return clp_config


def generate_credentials_file(credentials_file_path: pathlib.Path):
    credentials = {
        DB_COMPONENT_NAME: {"user": "clp-user", "password": secrets.token_urlsafe(8)},
        QUEUE_COMPONENT_NAME: {"user": "clp-user", "password": secrets.token_urlsafe(8)},
        REDIS_COMPONENT_NAME: {"password": secrets.token_urlsafe(16)},
    }

    with open(credentials_file_path, "w") as f:
        yaml.safe_dump(credentials, f)


def validate_credentials_file_path(
    clp_config: CLPConfig, clp_home: pathlib.Path, generate_default_file: bool
):
    credentials_file_path = clp_config.credentials_file_path
    if not credentials_file_path.exists():
        if (
            make_config_path_absolute(clp_home, CLP_DEFAULT_CREDENTIALS_FILE_PATH)
            == credentials_file_path
            and generate_default_file
        ):
            generate_credentials_file(credentials_file_path)
        else:
            raise ValueError(f"Credentials file path '{credentials_file_path}' does not exist.")
    elif not credentials_file_path.is_file():
        raise ValueError(f"Credentials file path '{credentials_file_path}' is not a file.")


def validate_and_load_db_credentials_file(
    clp_config: CLPConfig, clp_home: pathlib.Path, generate_default_file: bool
):
    validate_credentials_file_path(clp_config, clp_home, generate_default_file)
    clp_config.load_database_credentials_from_file()


def validate_and_load_queue_credentials_file(
    clp_config: CLPConfig, clp_home: pathlib.Path, generate_default_file: bool
):
    validate_credentials_file_path(clp_config, clp_home, generate_default_file)
    clp_config.load_queue_credentials_from_file()


def validate_and_load_redis_credentials_file(
    clp_config: CLPConfig, clp_home: pathlib.Path, generate_default_file: bool
):
    validate_credentials_file_path(clp_config, clp_home, generate_default_file)
    clp_config.load_redis_credentials_from_file()


def validate_db_config(clp_config: CLPConfig, data_dir: pathlib.Path, logs_dir: pathlib.Path):
    try:
        validate_path_could_be_dir(data_dir)
    except ValueError as ex:
        raise ValueError(f"{DB_COMPONENT_NAME} data directory is invalid: {ex}")

    try:
        validate_path_could_be_dir(logs_dir)
    except ValueError as ex:
        raise ValueError(f"{DB_COMPONENT_NAME} logs directory is invalid: {ex}")

    validate_port(f"{DB_COMPONENT_NAME}.port", clp_config.database.host, clp_config.database.port)


def validate_queue_config(clp_config: CLPConfig, logs_dir: pathlib.Path):
    try:
        validate_path_could_be_dir(logs_dir)
    except ValueError as ex:
        raise ValueError(f"{QUEUE_COMPONENT_NAME} logs directory is invalid: {ex}")

    validate_port(f"{QUEUE_COMPONENT_NAME}.port", clp_config.queue.host, clp_config.queue.port)


def validate_redis_config(
    clp_config: CLPConfig, data_dir: pathlib.Path, logs_dir: pathlib.Path, base_config: pathlib.Path
):
    try:
        validate_path_could_be_dir(data_dir)
    except ValueError as ex:
        raise ValueError(f"{REDIS_COMPONENT_NAME} data directory is invalid {ex}")

    try:
        validate_path_could_be_dir(logs_dir)
    except ValueError as ex:
        raise ValueError(f"{REDIS_COMPONENT_NAME} logs directory is invalid: {ex}")

    if not base_config.exists():
        raise ValueError(
            f"{REDIS_COMPONENT_NAME} base configuration at {str(base_config)} is missing."
        )

    validate_port(f"{REDIS_COMPONENT_NAME}.port", clp_config.redis.host, clp_config.redis.port)


def validate_reducer_config(clp_config: CLPConfig, logs_dir: pathlib.Path, num_workers: int):
    try:
        validate_path_could_be_dir(logs_dir)
    except ValueError as ex:
        raise ValueError(f"{REDUCER_COMPONENT_NAME} logs directory is invalid: {ex}")

    for i in range(0, num_workers):
        validate_port(
            f"{REDUCER_COMPONENT_NAME}.port",
            clp_config.reducer.host,
            clp_config.reducer.base_port + i,
        )


def validate_results_cache_config(
    clp_config: CLPConfig, data_dir: pathlib.Path, logs_dir: pathlib.Path
):
    try:
        validate_path_could_be_dir(data_dir)
    except ValueError as ex:
        raise ValueError(f"{RESULTS_CACHE_COMPONENT_NAME} data directory is invalid: {ex}")

    try:
        validate_path_could_be_dir(logs_dir)
    except ValueError as ex:
        raise ValueError(f"{RESULTS_CACHE_COMPONENT_NAME} logs directory is invalid: {ex}")

    validate_port(
        f"{RESULTS_CACHE_COMPONENT_NAME}.port",
        clp_config.results_cache.host,
        clp_config.results_cache.port,
    )


def validate_worker_config(clp_config: CLPConfig):
    clp_config.validate_input_logs_dir()
    clp_config.validate_archive_output_config()
    clp_config.validate_stream_output_dir()


def validate_webui_config(
    clp_config: CLPConfig, logs_dir: pathlib.Path, settings_json_path: pathlib.Path
):
    if not settings_json_path.exists():
        raise ValueError(
            f"{WEBUI_COMPONENT_NAME} {settings_json_path} is not a valid path to Meteor settings.json"
        )

    try:
        validate_path_could_be_dir(logs_dir)
    except ValueError as ex:
        raise ValueError(f"{WEBUI_COMPONENT_NAME} logs directory is invalid: {ex}")

    validate_port(f"{WEBUI_COMPONENT_NAME}.port", clp_config.webui.host, clp_config.webui.port)


def validate_log_viewer_webui_config(clp_config: CLPConfig, settings_json_path: pathlib.Path):
    if not settings_json_path.exists():
        raise ValueError(
            f"{WEBUI_COMPONENT_NAME} {settings_json_path} is not a valid path to settings.json"
        )

    validate_port(
        f"{LOG_VIEWER_WEBUI_COMPONENT_NAME}.port",
        clp_config.log_viewer_webui.host,
        clp_config.log_viewer_webui.port,
    )
