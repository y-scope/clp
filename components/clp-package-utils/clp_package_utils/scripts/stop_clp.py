import argparse
import logging
import pathlib
import subprocess
import sys
from typing import List

from clp_py_utils.clp_config import (
    ALL_TARGET_NAME,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    CONTROLLER_TARGET_NAME,
    DB_COMPONENT_NAME,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    QUEUE_COMPONENT_NAME,
    REDIS_COMPONENT_NAME,
    REDUCER_COMPONENT_NAME,
    RESULTS_CACHE_COMPONENT_NAME,
    WEBUI_COMPONENT_NAME,
)

from clp_package_utils.general import (
    CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH,
    get_clp_home,
    is_container_exited,
    is_container_running,
    load_config_file,
    validate_and_load_db_credentials_file,
    validate_and_load_queue_credentials_file,
)

logger = logging.getLogger(__file__)


def stop_running_container(container_name: str, already_exited_containers: List[str], force: bool):
    if is_container_running(container_name):
        logger.info(f"Stopping {container_name}...")
        cmd = ["docker", "stop", container_name]
        subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

        logger.info(f"Removing {container_name}...")
        cmd = ["docker", "rm", container_name]
        subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)

        logger.info(f"Stopped and removed {container_name}.")
    elif is_container_exited(container_name):
        if force:
            logger.info(f"Forcibly removing exited {container_name}...")
            cmd = ["docker", "rm", container_name]
            subprocess.run(cmd, stdout=subprocess.DEVNULL, check=True)
            logger.info(f"Removed {container_name}...")
        else:
            already_exited_containers.append(container_name)


def main(argv):
    clp_home = get_clp_home()
    default_config_file_path = clp_home / CLP_DEFAULT_CONFIG_FILE_RELATIVE_PATH

    args_parser = argparse.ArgumentParser(description="Stops CLP")
    args_parser.add_argument(
        "--config",
        "-c",
        default=str(default_config_file_path),
        help="CLP package configuration file.",
    )
    args_parser.add_argument(
        "--force",
        "-f",
        action="store_true",
        help="Forcibly remove exited containers",
    )

    component_args_parser = args_parser.add_subparsers(dest="target")
    component_args_parser.add_parser(CONTROLLER_TARGET_NAME)
    component_args_parser.add_parser(DB_COMPONENT_NAME)
    component_args_parser.add_parser(QUEUE_COMPONENT_NAME)
    component_args_parser.add_parser(REDIS_COMPONENT_NAME)
    component_args_parser.add_parser(REDUCER_COMPONENT_NAME)
    component_args_parser.add_parser(RESULTS_CACHE_COMPONENT_NAME)
    component_args_parser.add_parser(COMPRESSION_SCHEDULER_COMPONENT_NAME)
    component_args_parser.add_parser(QUERY_SCHEDULER_COMPONENT_NAME)
    component_args_parser.add_parser(COMPRESSION_WORKER_COMPONENT_NAME)
    component_args_parser.add_parser(QUERY_WORKER_COMPONENT_NAME)
    component_args_parser.add_parser(WEBUI_COMPONENT_NAME)

    parsed_args = args_parser.parse_args(argv[1:])

    if parsed_args.target:
        target = parsed_args.target
    else:
        target = ALL_TARGET_NAME

    # Validate and load config file
    try:
        config_file_path = pathlib.Path(parsed_args.config)
        clp_config = load_config_file(config_file_path, default_config_file_path, clp_home)

        # Validate and load necessary credentials
        if target in (
            ALL_TARGET_NAME,
            CONTROLLER_TARGET_NAME,
            DB_COMPONENT_NAME,
        ):
            validate_and_load_db_credentials_file(clp_config, clp_home, False)
        if target in (
            ALL_TARGET_NAME,
            CONTROLLER_TARGET_NAME,
            COMPRESSION_SCHEDULER_COMPONENT_NAME,
            COMPRESSION_WORKER_COMPONENT_NAME,
            QUEUE_COMPONENT_NAME,
            QUERY_SCHEDULER_COMPONENT_NAME,
            QUERY_WORKER_COMPONENT_NAME,
        ):
            validate_and_load_queue_credentials_file(clp_config, clp_home, False)
    except:
        logger.exception("Failed to load config.")
        return -1

    try:
        # Read instance ID from file
        logs_dir = clp_config.logs_directory
        instance_id_file_path = logs_dir / "instance-id"
        if not (logs_dir.exists() and logs_dir.is_dir() and instance_id_file_path.exists()):
            # No instance ID file, so nothing to do
            return 0
        with open(instance_id_file_path, "r") as f:
            instance_id = f.readline()

        already_exited_containers = []
        force = parsed_args.force
        if target in (ALL_TARGET_NAME, WEBUI_COMPONENT_NAME):
            container_name = f"clp-{WEBUI_COMPONENT_NAME}-{instance_id}"
            stop_running_container(container_name, already_exited_containers, force)
        if target in (ALL_TARGET_NAME, REDUCER_COMPONENT_NAME):
            container_name = f"clp-{REDUCER_COMPONENT_NAME}-{instance_id}"
            stop_running_container(container_name, already_exited_containers, force)

            container_config_file_path = logs_dir / f"{container_name}.yml"
            if container_config_file_path.exists():
                container_config_file_path.unlink()
        if target in (ALL_TARGET_NAME, QUERY_WORKER_COMPONENT_NAME):
            container_name = f"clp-{QUERY_WORKER_COMPONENT_NAME}-{instance_id}"
            stop_running_container(container_name, already_exited_containers, force)
        if target in (ALL_TARGET_NAME, COMPRESSION_WORKER_COMPONENT_NAME):
            container_name = f"clp-{COMPRESSION_WORKER_COMPONENT_NAME}-{instance_id}"
            stop_running_container(container_name, already_exited_containers, force)
        if target in (ALL_TARGET_NAME, CONTROLLER_TARGET_NAME, QUERY_SCHEDULER_COMPONENT_NAME):
            container_name = f"clp-{QUERY_SCHEDULER_COMPONENT_NAME}-{instance_id}"
            stop_running_container(container_name, already_exited_containers, force)

            container_config_file_path = logs_dir / f"{container_name}.yml"
            if container_config_file_path.exists():
                container_config_file_path.unlink()
        if target in (
            ALL_TARGET_NAME,
            CONTROLLER_TARGET_NAME,
            COMPRESSION_SCHEDULER_COMPONENT_NAME,
        ):
            container_name = f"clp-{COMPRESSION_SCHEDULER_COMPONENT_NAME}-{instance_id}"
            stop_running_container(container_name, already_exited_containers, force)

            container_config_file_path = logs_dir / f"{container_name}.yml"
            if container_config_file_path.exists():
                container_config_file_path.unlink()
        if target in (ALL_TARGET_NAME, CONTROLLER_TARGET_NAME, REDIS_COMPONENT_NAME):
            container_name = f"clp-{REDIS_COMPONENT_NAME}-{instance_id}"
            stop_running_container(container_name, already_exited_containers, force)

            redis_config_file_path = logs_dir / f"{container_name}.conf"
            if redis_config_file_path.exists():
                redis_config_file_path.unlink()
        if target in (ALL_TARGET_NAME, RESULTS_CACHE_COMPONENT_NAME):
            container_name = f"clp-{RESULTS_CACHE_COMPONENT_NAME}-{instance_id}"
            stop_running_container(container_name, already_exited_containers, force)
        if target in (ALL_TARGET_NAME, CONTROLLER_TARGET_NAME, QUEUE_COMPONENT_NAME):
            container_name = f"clp-{QUEUE_COMPONENT_NAME}-{instance_id}"
            stop_running_container(container_name, already_exited_containers, force)

            queue_config_file_path = logs_dir / f"{container_name}.conf"
            if queue_config_file_path.exists():
                queue_config_file_path.unlink()
        if target in (ALL_TARGET_NAME, DB_COMPONENT_NAME):
            container_name = f"clp-{DB_COMPONENT_NAME}-{instance_id}"
            stop_running_container(container_name, already_exited_containers, force)

        if already_exited_containers:
            container_list = " ".join(already_exited_containers)
            logger.warning(
                f"The following containers have already exited and were not removed:"
                f" {container_list}"
            )
            logger.warning(f"Run with --force to remove them")
        elif target in ALL_TARGET_NAME:
            # NOTE: We can only remove the instance ID file if all containers have been stopped.
            # Currently, we only remove the instance file when all containers are stopped at once.
            # If a single container is stopped, it's expensive to check if the others are running,
            # so instead we don't remove the instance file. In the worst case, a user will have to
            # remove it manually.
            instance_id_file_path.unlink()
    except:
        logger.exception("Failed to stop CLP.")
        return -1

    return 0


if "__main__" == __name__:
    sys.exit(main(sys.argv))
