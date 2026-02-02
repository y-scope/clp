import enum
import errno
import json
import os
import pathlib
import re
import secrets
import socket
import subprocess
import uuid
from enum import auto

import yaml
from clp_py_utils.clp_config import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    CLP_DEFAULT_CREDENTIALS_FILE_PATH,
    CLP_SHARED_CONFIG_FILENAME,
    ClpConfig,
    CONTAINER_AWS_CONFIG_DIRECTORY,
    CONTAINER_CLP_HOME,
    CONTAINER_INPUT_LOGS_ROOT_DIR,
    DB_COMPONENT_NAME,
    MCP_SERVER_COMPONENT_NAME,
    QueryEngine,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    StorageType,
    WEBUI_COMPONENT_NAME,
    WorkerConfig,
)
from clp_py_utils.clp_metadata_db_utils import (
    MYSQL_TABLE_NAME_MAX_LEN,
    TABLE_SUFFIX_MAX_LEN,
)
from clp_py_utils.core import (
    get_config_value,
    make_config_path_absolute,
    read_yaml_config_file,
    resolve_host_path,
    resolve_host_path_in_container,
    validate_path_could_be_dir,
)
from strenum import KebabCaseStrEnum

# CONSTANTS
EXTRACT_FILE_CMD = "x"
EXTRACT_IR_CMD = "i"
EXTRACT_JSON_CMD = "j"

DOCKER_MOUNT_TYPE_STRINGS = ["bind"]

S3_KEY_PREFIX_COMPRESSION = "s3-key-prefix"
S3_OBJECT_COMPRESSION = "s3-object"


class DockerDependencyError(OSError):
    """Base class for errors related to Docker dependencies."""


class DockerNotAvailableError(DockerDependencyError):
    """Raised when Docker or Docker Compose is unavailable."""

    def __init__(self, base_message: str, process_error: subprocess.CalledProcessError) -> None:
        message = base_message
        output_chunks: list[str] = []
        for stream in (process_error.stdout, process_error.stderr):
            if stream is None:
                continue
            if isinstance(stream, bytes):
                text = stream.decode(errors="replace")
            else:
                text = str(stream)
            text = text.strip()
            if text:
                output_chunks.append(text)
        if len(output_chunks) > 0:
            message = "\n".join([base_message, *output_chunks])
        super().__init__(errno.ENOENT, message)


class DockerComposeProjectNotRunningError(DockerDependencyError):
    """Raised when a Docker Compose project is not running but should be."""

    def __init__(self, project_name: str) -> None:
        super().__init__(errno.ESRCH, f"Docker Compose project '{project_name}' is not running.")


class DockerComposeProjectAlreadyRunningError(DockerDependencyError):
    """Raised when a Docker Compose project is already running but should not be."""

    def __init__(self, project_name: str) -> None:
        super().__init__(
            errno.EEXIST, f"Docker Compose project '{project_name}' is already running."
        )


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


class ClpDockerMounts:
    def __init__(self, clp_home: pathlib.Path, docker_clp_home: pathlib.Path):
        self.input_logs_dir: DockerMount | None = None
        self.clp_home: DockerMount | None = DockerMount(
            DockerMountType.BIND, clp_home, docker_clp_home
        )
        self.data_dir: DockerMount | None = None
        self.logs_dir: DockerMount | None = None
        self.archives_output_dir: DockerMount | None = None
        self.stream_output_dir: DockerMount | None = None
        self.aws_config_dir: DockerMount | None = None
        self.generated_config_file: DockerMount | None = None


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
    if not clp_home.exists():
        raise ValueError("CLP_HOME set to nonexistent path.")

    return clp_home.resolve()


def generate_container_name(job_type: str) -> str:
    """
    :param job_type:
    :return: A unique container name for the given job type.
    """
    return f"clp-{job_type}-{uuid.uuid4()}"


def check_docker_dependencies(should_compose_project_be_running: bool, project_name: str):
    """
    Checks if Docker and Docker Compose are installed, and whether a Docker Compose project is
    running.

    :param should_compose_project_be_running:
    :param project_name: The Docker Compose project name to check.
    :raise DockerNotAvailableError: If any Docker dependency is not installed.
    :raise DockerComposeProjectNotRunningError: If the project isn't running when it should be.
    :raise DockerComposeProjectAlreadyRunningError: If the project is running when it shouldn't be.
    """
    try:
        subprocess.check_output(
            ["docker", "--version"],
            stderr=subprocess.STDOUT,
        )
    except subprocess.CalledProcessError as e:
        raise DockerNotAvailableError("docker is not installed or available on the path", e) from e

    is_running = _is_docker_compose_project_running(project_name)
    if should_compose_project_be_running and not is_running:
        raise DockerComposeProjectNotRunningError(project_name)
    if not should_compose_project_be_running and is_running:
        raise DockerComposeProjectAlreadyRunningError(project_name)


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
    clp_config: ClpConfig, clp_home: pathlib.Path
) -> tuple[ClpConfig, ClpDockerMounts]:
    """
    Copies the given config and sets up mounts mapping the relevant host paths into the container

    :param clp_config:
    :param clp_home:
    :return: The container config and the mounts.
    """
    container_clp_config = clp_config.model_copy(deep=True)

    docker_mounts = ClpDockerMounts(clp_home, CONTAINER_CLP_HOME)

    if StorageType.FS == clp_config.logs_input.type:
        input_logs_dir = resolve_host_path(clp_config.logs_input.directory)
        container_clp_config.logs_input.directory = (
            CONTAINER_INPUT_LOGS_ROOT_DIR / input_logs_dir.relative_to(input_logs_dir.anchor)
        )
        docker_mounts.input_logs_dir = DockerMount(
            DockerMountType.BIND, input_logs_dir, container_clp_config.logs_input.directory, True
        )

    if not is_path_already_mounted(
        clp_home, CONTAINER_CLP_HOME, clp_config.data_directory, container_clp_config.data_directory
    ):
        docker_mounts.data_dir = DockerMount(
            DockerMountType.BIND, clp_config.data_directory, container_clp_config.data_directory
        )

    if not is_path_already_mounted(
        clp_home, CONTAINER_CLP_HOME, clp_config.logs_directory, container_clp_config.logs_directory
    ):
        docker_mounts.logs_dir = DockerMount(
            DockerMountType.BIND, clp_config.logs_directory, container_clp_config.logs_directory
        )

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

    if not is_path_already_mounted(
        clp_home,
        CONTAINER_CLP_HOME,
        clp_config.get_shared_config_file_path(),
        container_clp_config.get_shared_config_file_path(),
    ):
        docker_mounts.generated_config_file = DockerMount(
            DockerMountType.BIND,
            clp_config.get_shared_config_file_path(),
            container_clp_config.get_shared_config_file_path(),
        )

    # Only create the mount if the directory exists
    if clp_config.aws_config_directory is not None:
        container_clp_config.aws_config_directory = CONTAINER_AWS_CONFIG_DIRECTORY
        docker_mounts.aws_config_dir = DockerMount(
            DockerMountType.BIND,
            clp_config.aws_config_directory,
            container_clp_config.aws_config_directory,
        )
    return container_clp_config, docker_mounts


def generate_docker_compose_container_config(clp_config: ClpConfig) -> ClpConfig:
    """
    Copies the given config and transforms mount paths and hosts for Docker Compose.

    :param clp_config:
    :return: The container config.
    """
    container_clp_config = clp_config.model_copy(deep=True)
    container_clp_config.transform_for_container()

    return container_clp_config


def generate_worker_config(clp_config: ClpConfig) -> WorkerConfig:
    worker_config = WorkerConfig()
    worker_config.package = clp_config.package.model_copy(deep=True)
    worker_config.archive_output = clp_config.archive_output.model_copy(deep=True)
    worker_config.tmp_directory = clp_config.tmp_directory

    worker_config.stream_output = clp_config.stream_output
    worker_config.stream_collection_name = clp_config.results_cache.stream_collection_name

    return worker_config


def get_container_config_filename(container_name: str) -> str:
    return f".{container_name}-config.yaml"


def dump_container_config(
    container_clp_config: ClpConfig, clp_config: ClpConfig, config_filename: str
):
    """
    Writes the given container config to the logs directory, so that it's accessible in the
    container.

    :param container_clp_config: The config to write.
    :param clp_config: The corresponding config on the host (used to determine the logs directory).
    :param config_filename:
    :return: The path to the config file in the container and on the host.
    """
    config_file_path_on_host = clp_config.logs_directory / config_filename
    config_file_path_on_container = container_clp_config.logs_directory / config_filename
    resolved_config_file_path_on_host = resolve_host_path_in_container(config_file_path_on_host)
    with open(resolved_config_file_path_on_host, "w") as f:
        yaml.safe_dump(container_clp_config.dump_to_primitive_dict(), f)

    return config_file_path_on_container, config_file_path_on_host


def dump_shared_container_config(container_clp_config: ClpConfig, clp_config: ClpConfig):
    """
    Dumps the given container config to `CLP_SHARED_CONFIG_FILENAME` in the logs directory, so that
    it's accessible in the container.

    :param container_clp_config:
    :param clp_config:
    """
    dump_container_config(container_clp_config, clp_config, CLP_SHARED_CONFIG_FILENAME)


def generate_container_start_cmd(
    container_name: str,
    container_mounts: list[DockerMount | None],
    container_image: str,
    extra_env_vars: dict[str, str] | None = None,
) -> list[str]:
    """
    Generates the command to start a container with the given mounts, environment variables, and
    name.

    :param container_name:
    :param container_mounts:
    :param container_image:
    :param extra_env_vars: Environment variables to set on top of the predefined ones.
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
        "-u", f"{os.getuid()}:{os.getgid()}",
        "--name", container_name,
        "--log-driver", "local"
    ]
    env_vars = {
        "PYTHONPATH": clp_site_packages_dir,
        **(extra_env_vars if extra_env_vars is not None else {}),
    }
    for key, value in env_vars.items():
        container_start_cmd.extend(["-e", f"{key}={value}"])
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


def load_config_file(config_file_path: pathlib.Path) -> ClpConfig:
    """
    Loads and validates a CLP configuration file.

    :param config_file_path:
    :return: The loaded and validated ClpConfig object.
    :raise ValueError: If the specified config file does not exist, and the requested path is not
    the path to the default config file.
    """
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    if config_file_path.exists():
        raw_clp_config = read_yaml_config_file(config_file_path)
        if raw_clp_config is None:
            clp_config = ClpConfig()
        else:
            clp_config = ClpConfig.model_validate(raw_clp_config)
    elif config_file_path != default_config_file_path:
        err_msg = f"Config file '{config_file_path}' does not exist."
        raise ValueError(err_msg)
    else:
        clp_config = ClpConfig()

    clp_config.make_config_paths_absolute(clp_home)
    clp_config.load_container_image_ref()

    validate_path_for_container_mount(clp_config.data_directory)
    validate_path_for_container_mount(clp_config.logs_directory)
    validate_path_for_container_mount(clp_config.tmp_directory)

    return clp_config


def generate_credentials_file(credentials_file_path: pathlib.Path):
    credentials = {
        DB_COMPONENT_NAME: {
            "username": "clp-user",
            "password": secrets.token_urlsafe(8),
            "root_username": "root",
            "root_password": secrets.token_urlsafe(8),
            "spider_username": "spider-user",
            "spider_password": secrets.token_urlsafe(8),
        },
        QUEUE_COMPONENT_NAME: {"username": "clp-user", "password": secrets.token_urlsafe(8)},
        REDIS_COMPONENT_NAME: {"password": secrets.token_urlsafe(16)},
    }

    with open(credentials_file_path, "w") as f:
        yaml.safe_dump(credentials, f)


def validate_credentials_file_path(
    clp_config: ClpConfig, clp_home: pathlib.Path, generate_default_file: bool
):
    credentials_file_path = clp_config.credentials_file_path
    resolved_credentials_file_path = resolve_host_path_in_container(credentials_file_path)
    if not resolved_credentials_file_path.exists():
        if (
            make_config_path_absolute(clp_home, CLP_DEFAULT_CREDENTIALS_FILE_PATH)
            == credentials_file_path
            and generate_default_file
        ):
            generate_credentials_file(resolved_credentials_file_path)
        else:
            raise ValueError(f"Credentials file path '{credentials_file_path}' does not exist.")
    elif not resolved_credentials_file_path.is_file():
        raise ValueError(f"Credentials file path '{credentials_file_path}' is not a file.")


def validate_and_load_db_credentials_file(
    clp_config: ClpConfig, clp_home: pathlib.Path, generate_default_file: bool
):
    validate_credentials_file_path(clp_config, clp_home, generate_default_file)
    clp_config.database.load_credentials_from_file(clp_config.credentials_file_path)


def validate_and_load_queue_credentials_file(
    clp_config: ClpConfig, clp_home: pathlib.Path, generate_default_file: bool
):
    validate_credentials_file_path(clp_config, clp_home, generate_default_file)
    clp_config.queue.load_credentials_from_file(clp_config.credentials_file_path)


def validate_and_load_redis_credentials_file(
    clp_config: ClpConfig, clp_home: pathlib.Path, generate_default_file: bool
):
    validate_credentials_file_path(clp_config, clp_home, generate_default_file)
    clp_config.redis.load_credentials_from_file(clp_config.credentials_file_path)


def validate_db_config(
    clp_config: ClpConfig,
    component_config: pathlib.Path,
    data_dir: pathlib.Path,
    logs_dir: pathlib.Path,
):
    resolved_component_config = resolve_host_path_in_container(component_config)
    if not resolved_component_config.exists():
        raise ValueError(f"{DB_COMPONENT_NAME} configuration file missing: '{component_config}'.")
    _validate_data_directory(data_dir, DB_COMPONENT_NAME)
    _validate_log_directory(logs_dir, DB_COMPONENT_NAME)

    validate_port(f"{DB_COMPONENT_NAME}.port", clp_config.database.host, clp_config.database.port)


def validate_queue_config(clp_config: ClpConfig, logs_dir: pathlib.Path):
    _validate_log_directory(logs_dir, QUEUE_COMPONENT_NAME)

    validate_port(f"{QUEUE_COMPONENT_NAME}.port", clp_config.queue.host, clp_config.queue.port)


def validate_redis_config(
    clp_config: ClpConfig,
    component_config: pathlib.Path,
    data_dir: pathlib.Path,
    logs_dir: pathlib.Path,
):
    resolved_component_config = resolve_host_path_in_container(component_config)
    if not resolved_component_config.exists():
        raise ValueError(
            f"{REDIS_COMPONENT_NAME} configuration file missing: '{component_config}'."
        )
    _validate_data_directory(data_dir, REDIS_COMPONENT_NAME)
    _validate_log_directory(logs_dir, REDIS_COMPONENT_NAME)

    validate_port(f"{REDIS_COMPONENT_NAME}.port", clp_config.redis.host, clp_config.redis.port)


def validate_reducer_config(clp_config: ClpConfig, logs_dir: pathlib.Path, num_workers: int):
    _validate_log_directory(logs_dir, REDUCER_COMPONENT_NAME)

    for i in range(num_workers):
        validate_port(
            f"{REDUCER_COMPONENT_NAME}.port",
            clp_config.reducer.host,
            clp_config.reducer.base_port + i,
        )


def validate_results_cache_config(
    clp_config: ClpConfig,
    component_config: pathlib.Path,
    data_dir: pathlib.Path,
    logs_dir: pathlib.Path,
):
    resolved_component_config = resolve_host_path_in_container(component_config)
    if not resolved_component_config.exists():
        raise ValueError(
            f"{RESULTS_CACHE_COMPONENT_NAME} configuration file missing: '{component_config}'."
        )
    _validate_data_directory(data_dir, RESULTS_CACHE_COMPONENT_NAME)
    _validate_log_directory(logs_dir, RESULTS_CACHE_COMPONENT_NAME)

    validate_port(
        f"{RESULTS_CACHE_COMPONENT_NAME}.port",
        clp_config.results_cache.host,
        clp_config.results_cache.port,
    )


def validate_output_storage_config(clp_config: ClpConfig) -> None:
    clp_config.validate_archive_output_config(True)
    clp_config.validate_stream_output_config(True)

    validate_path_for_container_mount(clp_config.archive_output.get_directory())
    validate_path_for_container_mount(clp_config.stream_output.get_directory())


def validate_webui_config(
    clp_config: ClpConfig,
    client_settings_json_path: pathlib.Path,
    server_settings_json_path: pathlib.Path,
):
    for path in [client_settings_json_path, server_settings_json_path]:
        resolved_path = resolve_host_path_in_container(path)
        if not resolved_path.exists():
            raise ValueError(f"{WEBUI_COMPONENT_NAME} {path} is not a valid path to settings.json")

    validate_port(f"{WEBUI_COMPONENT_NAME}.port", clp_config.webui.host, clp_config.webui.port)


def validate_mcp_server_config(clp_config: ClpConfig, logs_dir: pathlib.Path):
    _validate_log_directory(logs_dir, MCP_SERVER_COMPONENT_NAME)

    validate_port(
        f"{MCP_SERVER_COMPONENT_NAME}.port", clp_config.mcp_server.host, clp_config.mcp_server.port
    )


def validate_path_for_container_mount(path: pathlib.Path) -> None:
    RESTRICTED_PREFIXES: list[pathlib.Path] = [
        CONTAINER_AWS_CONFIG_DIRECTORY,
        CONTAINER_CLP_HOME,
        CONTAINER_INPUT_LOGS_ROOT_DIR,
        pathlib.Path("/bin"),
        pathlib.Path("/boot"),
        pathlib.Path("/dev"),
        pathlib.Path("/etc"),
        pathlib.Path("/lib"),
        pathlib.Path("/lib32"),
        pathlib.Path("/lib64"),
        pathlib.Path("/libx32"),
        pathlib.Path("/proc"),
        pathlib.Path("/root"),
        pathlib.Path("/run"),
        pathlib.Path("/sbin"),
        pathlib.Path("/srv"),
        pathlib.Path("/sys"),
        pathlib.Path("/usr"),
        pathlib.Path("/var"),
    ]

    if not path.is_absolute():
        raise ValueError(f"Path: `{path}` must be absolute:")

    for prefix in RESTRICTED_PREFIXES:
        if path.is_relative_to(prefix):
            raise ValueError(
                f"Invalid path: `{path}` cannot be under '{prefix}' which may overlap with a path"
                f" in the container."
            )


def validate_dataset_name(clp_table_prefix: str, dataset_name: str) -> None:
    """
    Validates that the given dataset name abides by the following rules:
    - Its length won't cause any metadata table names to exceed MySQL's max table name length.
    - It only contains alphanumeric characters and underscores.

    :param clp_table_prefix:
    :param dataset_name:
    :raise: ValueError if the dataset name is invalid.
    """
    if re.fullmatch(r"\w+", dataset_name) is None:
        raise ValueError(
            f"Invalid dataset name: `{dataset_name}`. Names can only contain alphanumeric"
            f" characters and underscores."
        )

    dataset_name_max_len = (
        MYSQL_TABLE_NAME_MAX_LEN
        - len(clp_table_prefix)
        - 1  # For the separator between the dataset name and the table suffix
        - TABLE_SUFFIX_MAX_LEN
    )
    if len(dataset_name) > dataset_name_max_len:
        raise ValueError(
            f"Invalid dataset name: `{dataset_name}`. Names can only be a maximum of"
            f" {dataset_name_max_len} characters long."
        )


def validate_retention_config(clp_config: ClpConfig) -> None:
    clp_query_engine = clp_config.package.query_engine
    if is_retention_period_configured(clp_config) and clp_query_engine == QueryEngine.PRESTO:
        raise ValueError(
            f"Retention control is not supported with query_engine `{clp_query_engine}`"
        )


def is_retention_period_configured(clp_config: ClpConfig) -> bool:
    if clp_config.archive_output.retention_period is not None:
        return True

    if clp_config.results_cache.retention_period is not None:
        return True

    return False


def get_common_env_vars_list(
    include_clp_home_env_var=True,
) -> list[str]:
    """
    :param include_clp_home_env_var:
    :return: A list of common environment variables for Docker containers, in the format
    "KEY=VALUE".
    """
    clp_site_packages_dir = CONTAINER_CLP_HOME / "lib" / "python3" / "site-packages"
    env_vars = [f"PYTHONPATH={clp_site_packages_dir}"]

    if include_clp_home_env_var:
        env_vars.append(f"CLP_HOME={CONTAINER_CLP_HOME}")

    return env_vars


def get_credential_env_vars_list(
    container_clp_config: ClpConfig,
    include_db_credentials=False,
    include_queue_credentials=False,
    include_redis_credentials=False,
) -> list[str]:
    """
    :param container_clp_config:
    :param include_db_credentials:
    :param include_queue_credentials:
    :param include_redis_credentials:
    :return: A list of credential environment variables for Docker containers, in the format
    "KEY=VALUE".
    """
    env_vars = []

    if include_db_credentials:
        env_vars.append(f"CLP_DB_USER={container_clp_config.database.username}")
        env_vars.append(f"CLP_DB_PASS={container_clp_config.database.password}")

    if include_queue_credentials:
        env_vars.append(f"CLP_QUEUE_USER={container_clp_config.queue.username}")
        env_vars.append(f"CLP_QUEUE_PASS={container_clp_config.queue.password}")

    if include_redis_credentials:
        env_vars.append(f"CLP_REDIS_PASS={container_clp_config.redis.password}")

    return env_vars


def get_celery_connection_env_vars_list(container_clp_config: ClpConfig) -> list[str]:
    """
    :param container_clp_config:
    :return: A list of Celery connection environment variables for Docker containers, in the format
    "KEY=VALUE".
    """
    env_vars = [
        f"BROKER_URL=amqp://"
        f"{container_clp_config.queue.username}:{container_clp_config.queue.password}@"
        f"{container_clp_config.queue.host}:{container_clp_config.queue.port}",
        f"RESULT_BACKEND=redis://default:{container_clp_config.redis.password}@"
        f"{container_clp_config.redis.host}:{container_clp_config.redis.port}/"
        f"{container_clp_config.redis.query_backend_database}",
    ]

    return env_vars


def _is_docker_compose_project_running(project_name: str) -> bool:
    """
    Checks if a Docker Compose project is running.

    :param project_name:
    :return: Whether at least one instance is running.
    :raise DockerNotAvailableError: If Docker Compose is not installed or fails. The error message
    includes the Docker command's output when available.
    """
    cmd = ["docker", "compose", "ls", "--format", "json", "--filter", f"name={project_name}"]
    try:
        output = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
        running_instances = json.loads(output)
        return len(running_instances) >= 1
    except subprocess.CalledProcessError as e:
        raise DockerNotAvailableError(
            "Docker Compose is not installed or not functioning properly.", e
        ) from e


def _validate_data_directory(data_dir: pathlib.Path, component_name: str) -> None:
    try:
        validate_path_could_be_dir(resolve_host_path_in_container(data_dir))
    except ValueError as ex:
        raise ValueError(f"{component_name} data directory is invalid: {ex}")


def _validate_log_directory(logs_dir: pathlib.Path, component_name: str):
    """
    Validates that the logs directory path for a component is valid.

    :param logs_dir:
    :param component_name:
    :raise ValueError: If the path is invalid or can't be a directory.
    """
    try:
        validate_path_could_be_dir(resolve_host_path_in_container(logs_dir))
    except ValueError as ex:
        raise ValueError(f"{component_name} logs directory is invalid: {ex}")
