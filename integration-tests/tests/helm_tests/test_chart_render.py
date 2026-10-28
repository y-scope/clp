"""Render tests for the CLP package Helm chart."""

import re
from typing import Any

import pytest

from tests.helm_tests.utils import (
    HelmTemplateError,
    parse_properties,
    RELEASE_NAME,
    render_chart,
    RenderedChart,
)

pytestmark = pytest.mark.helm

COORDINATOR = f"{RELEASE_NAME}-presto-coordinator"
WORKER = f"{RELEASE_NAME}-presto-worker"
CONFIG_MAP = f"{RELEASE_NAME}-config"
PLUGIN_DIR = "/opt/presto-server/plugin/clp"
CONNECTOR_REPOSITORY = "ghcr.io/y-scope/clp-plugin-presto-connector"

STATIC_CREDENTIALS = {
    "access_key_id": "AKIAEXAMPLEKEY",
    "secret_access_key": "example/secret+access=key",
}


def _presto_values() -> dict[str, Any]:
    """:return: Values that deploy the bundled Presto, with archives on the filesystem."""
    return {
        "clpConfig": {
            "webui": {"query_engine": "presto"},
            "presto": {
                "coordinator": {
                    "logging_level": "INFO",
                    "query_max_memory_gb": 1,
                    "query_max_memory_per_node_gb": 1,
                },
                "worker": {"query_memory_gb": 4, "system_memory_gb": 8},
                "split_filter": {},
            },
        },
    }


def _config_map_properties(chart: RenderedChart, key: str) -> dict[str, str]:
    return parse_properties(chart.get("ConfigMap", CONFIG_MAP)["data"][key])


@pytest.fixture(scope="module")
def presto_chart() -> RenderedChart:
    """:return: The chart rendered with the bundled Presto."""
    return render_chart(_presto_values())


@pytest.mark.parametrize(
    ("deployment_name", "container_name", "install_path_env"),
    [
        (COORDINATOR, "presto-coordinator", "COORDINATOR_PLUGIN_INSTALL_PATH"),
        (WORKER, "presto-worker", "WORKER_PLUGIN_INSTALL_PATH"),
    ],
)
def test_presto_pods_install_connector(
    presto_chart: RenderedChart,
    deployment_name: str,
    container_name: str,
    install_path_env: str,
) -> None:
    """
    Tests that each Presto pod copies the connector from its installer image into a volume that
    the Presto container loads as its CLP plugin.
    """
    installer = presto_chart.container(deployment_name, "install-clp-plugin")
    assert installer["image"].startswith(f"{CONNECTOR_REPOSITORY}:")
    assert "@sha256:" in installer["image"]
    (install_env,) = installer["env"]
    assert install_path_env == install_env["name"]
    assert [{"name": "presto-plugin", "mountPath": install_env["value"]}] == installer[
        "volumeMounts"
    ]

    presto = presto_chart.container(deployment_name, container_name)
    assert "@sha256:" in presto["image"]
    assert {"name": "presto-plugin", "mountPath": PLUGIN_DIR, "readOnly": True} in presto[
        "volumeMounts"
    ]
    assert {"name": "presto-plugin", "emptyDir": {}} in presto_chart.pod_spec(deployment_name)[
        "volumes"
    ]


def test_presto_loads_connector(presto_chart: RenderedChart) -> None:
    """
    Tests that the worker loads the connector from the plugin directory, and that the coordinator
    uses the connector's serialization codecs, which the worker needs to read the connector's
    handles.
    """
    coordinator_config = _config_map_properties(
        presto_chart, "presto-coordinator-config-config.properties"
    )
    assert "true" == coordinator_config["use-connector-provided-serialization-codecs"]
    worker_config = _config_map_properties(presto_chart, "presto-worker-config-config.properties")
    assert PLUGIN_DIR == worker_config["plugin.dir"]


def test_presto_reads_s3_archives_with_static_credentials() -> None:
    """Tests that the worker catalog keeps reading S3 archives with static credentials."""
    values = _presto_values()
    values["clpConfig"]["archive_output"] = {
        "storage": {
            "type": "s3",
            "s3_config": {
                "region_code": "ca-central-1",
                "bucket": "example-bucket",
                "key_prefix": "archives/",
                "aws_authentication": {
                    "type": "credentials",
                    "credentials": dict(STATIC_CREDENTIALS),
                },
            },
        },
    }
    chart = render_chart(values)
    assert {
        "connector.name": "clp",
        "clp.storage-type": "s3",
        "clp.s3-auth-provider": "clp_package",
        "clp.s3-access-key-id": STATIC_CREDENTIALS["access_key_id"],
        "clp.s3-secret-access-key": STATIC_CREDENTIALS["secret_access_key"],
        "clp.s3-bucket": "example-bucket",
        "clp.s3-end-point": "https://s3.ca-central-1.amazonaws.com",
    } == _config_map_properties(chart, "presto-worker-catalog-clp.properties")


@pytest.mark.parametrize(
    ("image", "expected_ref"),
    [
        ({"tag": "0.300", "digest": None}, "registry.example/presto:0.300"),
        ({"tag": None, "digest": "sha256:abc"}, "registry.example/presto@sha256:abc"),
        ({"tag": "0.300", "digest": "sha256:abc"}, "registry.example/presto:0.300@sha256:abc"),
    ],
)
def test_image_refs(image: dict[str, Any], expected_ref: str) -> None:
    """Tests that images can be selected by tag, digest, or both."""
    values = _presto_values()
    values["image"] = {
        "prestoCoordinator": {"repository": "registry.example/presto", **image},
    }
    chart = render_chart(values)
    assert expected_ref == chart.container(COORDINATOR, "presto-coordinator")["image"]


def test_image_ref_requires_tag_or_digest() -> None:
    """Tests that an image with neither a tag nor a digest fails to render."""
    values = _presto_values()
    values["image"] = {"clpConnector": {"tag": None, "digest": None}}
    error = 'image.clpConnector requires "tag", "digest", or both.'
    with pytest.raises(HelmTemplateError, match=re.escape(error)):
        render_chart(values)
