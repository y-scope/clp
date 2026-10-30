"""Utilities for rendering the CLP package Helm chart in tests."""

import subprocess
from collections.abc import Iterator
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import yaml

from tests.utils.utils import get_binary_path, resolve_path_env_var

RELEASE_NAME = "clp"


class HelmTemplateError(Exception):
    """Raised when `helm template` fails to render the chart."""


@dataclass(frozen=True)
class RenderedChart:
    """Kubernetes objects rendered from the chart."""

    objects: list[dict[str, Any]]

    def find(self, kind: str, name: str) -> dict[str, Any] | None:
        """
        :param kind:
        :param name:
        :return: The rendered object with the given kind and name, or None if there isn't one.
        """
        for obj in self.objects:
            if obj["kind"] == kind and obj["metadata"]["name"] == name:
                return obj
        return None

    def get(self, kind: str, name: str) -> dict[str, Any]:
        """
        :param kind:
        :param name:
        :return: The rendered object with the given kind and name.
        :raise AssertionError: if there's no such object.
        """
        obj = self.find(kind, name)
        assert obj is not None, f"{kind} '{name}' wasn't rendered."
        return obj

    def pod_spec(self, deployment_name: str) -> dict[str, Any]:
        """
        :param deployment_name:
        :return: The pod spec of the Deployment with the given name.
        """
        pod_spec: dict[str, Any] = self.get("Deployment", deployment_name)["spec"]["template"][
            "spec"
        ]
        return pod_spec

    def pod_containers(self, deployment_name: str) -> Iterator[dict[str, Any]]:
        """
        :param deployment_name:
        :return: An iterator over the Deployment's init containers and containers.
        """
        pod_spec = self.pod_spec(deployment_name)
        yield from pod_spec.get("initContainers", [])
        yield from pod_spec["containers"]

    def container(self, deployment_name: str, container_name: str) -> dict[str, Any]:
        """
        :param deployment_name:
        :param container_name:
        :return: The Deployment's init container or container with the given name.
        :raise AssertionError: if there's no such container.
        """
        for container in self.pod_containers(deployment_name):
            if container["name"] == container_name:
                return container
        err_msg = f"Deployment '{deployment_name}' has no container '{container_name}'."
        raise AssertionError(err_msg)


def get_chart_dir() -> Path:
    """:return: The path to the CLP package Helm chart."""
    return (
        resolve_path_env_var("INTEGRATION_TESTS_PROJECT_ROOT").parent
        / "tools"
        / "deployment"
        / "package-helm"
    )


def render_chart(values: dict[str, Any] | None = None) -> RenderedChart:
    """
    Renders the chart with `helm template`.

    NOTE: The chart's dependencies must already be in its `charts` directory (e.g., through
    `helm dependency update`).

    :param values: Values that override the chart's defaults.
    :return: The rendered objects.
    :raise HelmTemplateError: if `helm template` fails.
    """
    result = subprocess.run(
        [get_binary_path("helm"), "template", RELEASE_NAME, str(get_chart_dir()), "--values", "-"],
        input=yaml.safe_dump(values or {}),
        capture_output=True,
        text=True,
        check=False,
    )
    if 0 != result.returncode:
        raise HelmTemplateError(result.stderr)
    objects = [obj for obj in yaml.safe_load_all(result.stdout) if obj is not None]
    return RenderedChart(objects)


def parse_properties(content: str) -> dict[str, str]:
    """
    Parses a Presto `.properties` file whose values don't need unescaping.

    :param content:
    :return: A map from each property's key to its value.
    """
    properties: dict[str, str] = {}
    for line in content.splitlines():
        stripped_line = line.strip()
        if "" == stripped_line or stripped_line.startswith("#"):
            continue
        key, _, value = stripped_line.partition("=")
        properties[key] = value
    return properties
