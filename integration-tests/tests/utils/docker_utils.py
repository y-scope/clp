"""Provide utility functions related to the use of Docker during integration tests."""

import re
import subprocess

import pytest

DOCKER_STATUS_FIELD_VALS = [
    "created",
    "restarting",
    "running",
    "removing",
    "paused",
    "exited",
    "dead",
]


def list_prefixed_containers(docker_bin: str, prefix: str) -> list[str]:
    """Returns a list of docker containers whose names begin with `prefix`."""
    ps_proc = subprocess.run(
        [
            docker_bin,
            "ps",
            "-a",
            "--format",
            "{{.Names}}",
            "--filter",
            f"name={prefix}",
        ],
        capture_output=True,
        text=True,
        check=False,
    )

    if ps_proc.returncode != 0:
        err_out = (ps_proc.stderr or ps_proc.stdout or "").strip()
        err_msg = f"Error listing containers for prefix {prefix}: {err_out}"
        raise RuntimeError(err_msg)

    candidates: list[str] = []
    for line in (ps_proc.stdout or "").splitlines():
        name_candidate = line.strip()
        if re.fullmatch(re.escape(prefix) + r"\d+", name_candidate):
            candidates.append(name_candidate)

    return candidates


def inspect_container_state(docker_bin: str, name: str, desired_state: str) -> bool:
    """Return True if the container status equals `desired_state`, else False. Raises on errors."""
    if desired_state not in DOCKER_STATUS_FIELD_VALS:
        err_msg = f"Unsupported desired_state: {desired_state}"
        raise ValueError(err_msg)

    inspect_proc = subprocess.run(
        [docker_bin, "inspect", "--format", "{{.State.Status}}", name],
        capture_output=True,
        text=True,
        check=False,
    )

    if inspect_proc.returncode != 0:
        err_out = (inspect_proc.stderr or inspect_proc.stdout or "").strip()
        if "No such object" in err_out:
            pytest.fail(f"Component container not found: {name}")
        err_msg = f"Error inspecting container {name}: {err_out}"
        raise RuntimeError(err_msg)

    status = (inspect_proc.stdout or "").strip().lower()
    return status == desired_state
