"""Render tests for the CLP package Helm chart."""

import copy
from typing import Any

import pytest
from clp_py_utils.clp_config import ClpConfig, WorkerConfig

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
CATALOG_SECRET = f"{RELEASE_NAME}-presto-catalog"
CONFIG_MAP = f"{RELEASE_NAME}-config"
PLUGIN_DIR = "/opt/presto-server/plugin/clp"

# Loading the connector plugin can take minutes, especially on slow or emulated nodes.
MIN_STARTUP_PROBE_SECONDS = 300

NATIVE_QUERY_DEPLOYMENTS = {
    f"{RELEASE_NAME}-query-scheduler",
    f"{RELEASE_NAME}-query-worker",
    f"{RELEASE_NAME}-reducer",
}
NATIVE_QUERY_SERVICES = {f"{RELEASE_NAME}-query-scheduler", f"{RELEASE_NAME}-reducer"}
API_SERVER = f"{RELEASE_NAME}-api-server"

STATIC_CREDENTIALS = {
    "access_key_id": "AKIAEXAMPLEKEY",
    "secret_access_key": "example/secret+access=key",
    "session_token": "example/session+token==",
}


def _s3_storage(aws_authentication: dict[str, Any], key_prefix: str) -> dict[str, Any]:
    return {
        "type": "s3",
        "s3_config": {
            "region_code": "ca-central-1",
            "bucket": "example-bucket",
            "key_prefix": key_prefix,
            "aws_authentication": aws_authentication,
        },
    }


def _presto_values(aws_authentication: dict[str, Any]) -> dict[str, Any]:
    """
    :param aws_authentication: S3 authentication for logs input, archives, and streams.
    :return: Values that search only through Presto, with logs input, archives, and streams on S3.
    """
    return {
        "serviceAccount": {
            "annotations": {
                "eks.amazonaws.com/role-arn": "arn:aws:iam::<account-id>:role/<role-name>",
            },
        },
        "clpConfig": {
            "api_server": None,
            "query_scheduler": None,
            "query_worker": None,
            "reducer": None,
            "results_cache": {"retention_period": None},
            "webui": {"query_engine": "presto"},
            "logs_input": {"type": "s3", "aws_authentication": aws_authentication},
            "archive_output": {"storage": _s3_storage(aws_authentication, "archives/")},
            "stream_output": {"storage": _s3_storage(aws_authentication, "streams/")},
            "presto": {
                "coordinator": {
                    "logging_level": "INFO",
                    "jvm_heap_size": "1536M",
                    "query_max_memory_gb": 1,
                    "query_max_memory_per_node_gb": 0.25,
                },
                "worker": {"query_memory_gb": 1, "system_memory_gb": 2},
                "split_filter": {},
            },
        },
    }


def _default_auth_values() -> dict[str, Any]:
    return _presto_values({"type": "default"})


def _static_auth_values() -> dict[str, Any]:
    return _presto_values({"type": "credentials", "credentials": dict(STATIC_CREDENTIALS)})


def _env_names(container: dict[str, Any]) -> set[str]:
    return {env["name"] for env in container.get("env", [])}


def _catalog(chart: RenderedChart, key: str) -> dict[str, str]:
    return parse_properties(chart.get("Secret", CATALOG_SECRET)["stringData"][key])


def _worker_catalog(chart: RenderedChart) -> dict[str, str]:
    return _catalog(chart, "presto-worker-catalog-clp.properties")


def _annotations(chart: RenderedChart, deployment_name: str) -> dict[str, str]:
    deployment = chart.get("Deployment", deployment_name)
    annotations: dict[str, str] = deployment["spec"]["template"]["metadata"]["annotations"]
    return annotations


def _assert_valid_clp_config(chart: RenderedChart) -> ClpConfig:
    """
    Validates the generated config as CLP components load it: most as a `ClpConfig`, and
    compression and query workers as a `WorkerConfig`.

    :param chart:
    :return: The validated `ClpConfig`.
    """
    config = chart.clp_config()
    WorkerConfig.model_validate(config)
    return ClpConfig.model_validate(config)


@pytest.fixture(scope="module")
def default_chart() -> RenderedChart:
    """:return: The chart rendered with its default values."""
    return render_chart()


@pytest.fixture(scope="module")
def default_auth_chart() -> RenderedChart:
    """:return: The chart rendered for Presto reading S3 archives with default authentication."""
    return render_chart(_default_auth_values())


@pytest.fixture(scope="module")
def static_auth_chart() -> RenderedChart:
    """:return: The chart rendered for Presto reading S3 archives with static credentials."""
    return render_chart(_static_auth_values())


def test_default_values_keep_native_search(default_chart: RenderedChart) -> None:
    """Tests that the default values deploy native search and the API server, but no Presto."""
    deployments = default_chart.names("Deployment")
    assert NATIVE_QUERY_DEPLOYMENTS | {API_SERVER} <= deployments
    assert NATIVE_QUERY_SERVICES | {API_SERVER} <= default_chart.names("Service")
    assert {COORDINATOR, WORKER}.isdisjoint(deployments)
    assert default_chart.find("Secret", CATALOG_SECRET) is None

    clp_config = _assert_valid_clp_config(default_chart)
    assert clp_config.api_server is not None
    assert "clp-s" == clp_config.webui.query_engine


@pytest.mark.parametrize("chart_fixture", ["default_auth_chart", "static_auth_chart"])
def test_presto_only_deployment(chart_fixture: str, request: pytest.FixtureRequest) -> None:
    """
    Tests that the Presto Web UI can run without the native query pipeline and the API server, and
    that compression workers still accept the generated config.
    """
    chart: RenderedChart = request.getfixturevalue(chart_fixture)
    assert {COORDINATOR, WORKER, f"{RELEASE_NAME}-webui"} <= chart.names("Deployment")
    assert (NATIVE_QUERY_DEPLOYMENTS | {API_SERVER}).isdisjoint(chart.names("Deployment"))
    assert (NATIVE_QUERY_SERVICES | {API_SERVER}).isdisjoint(chart.names("Service"))

    clp_config = _assert_valid_clp_config(chart)
    assert clp_config.api_server is None
    assert "presto" == clp_config.webui.query_engine
    assert clp_config.presto is not None
    assert (COORDINATOR, 8889) == (clp_config.presto.host, clp_config.presto.port)

    # Topology telemetry reports the disabled query workers and reducers like the controller does.
    emitter = chart.get("Job", f"{RELEASE_NAME}-topology-metrics-emitter")
    payload = emitter["spec"]["template"]["spec"]["containers"][0]["command"][-2]
    for metric in ["query_worker_replicas", "query_worker_concurrency", "reducer_replicas"]:
        assert f'"clp.deployment.{metric}","gauge":{{"dataPoints":[{{"asInt":"0"' in payload


def test_presto_reads_s3_archives_with_default_auth(default_auth_chart: RenderedChart) -> None:
    """
    Tests that, with default S3 authentication, workers use the AWS default credential provider
    chain through the release's service account, without static credentials.
    """
    assert {
        "connector.name": "clp",
        "clp.storage-type": "s3",
        "clp.s3-auth-provider": "default",
        "clp.s3-bucket": "example-bucket",
        "clp.s3-end-point": "https://s3.ca-central-1.amazonaws.com",
    } == _worker_catalog(default_auth_chart)

    worker_pod_spec = default_auth_chart.get("Deployment", WORKER)["spec"]["template"]["spec"]
    assert f"{RELEASE_NAME}-service-account" == worker_pod_spec["serviceAccountName"]
    worker_env = default_auth_chart.container(WORKER, "presto-worker")["env"]
    assert {"name": "AWS_REGION", "value": "ca-central-1"} in worker_env
    assert {"name": "AWS_DEFAULT_REGION", "value": "ca-central-1"} in worker_env


def test_presto_reads_s3_archives_with_static_credentials(static_auth_chart: RenderedChart) -> None:
    """Tests that static S3 credentials, including a session token, reach the worker catalog."""
    worker_catalog = _worker_catalog(static_auth_chart)
    assert "clp_package" == worker_catalog["clp.s3-auth-provider"]
    assert STATIC_CREDENTIALS["access_key_id"] == worker_catalog["clp.s3-access-key-id"]
    assert STATIC_CREDENTIALS["secret_access_key"] == worker_catalog["clp.s3-secret-access-key"]
    assert STATIC_CREDENTIALS["session_token"] == worker_catalog["clp.s3-session-token"]
    assert "AWS_REGION" not in _env_names(static_auth_chart.container(WORKER, "presto-worker"))


@pytest.mark.parametrize("chart_fixture", ["default_auth_chart", "static_auth_chart"])
def test_presto_catalogs_are_secret(chart_fixture: str, request: pytest.FixtureRequest) -> None:
    """Tests that the catalogs, which contain credentials, are only in the catalog Secret."""
    chart: RenderedChart = request.getfixturevalue(chart_fixture)
    coordinator_catalog = _catalog(chart, "presto-coordinator-catalog-clp.properties")
    assert "pass" == coordinator_catalog["clp.metadata-db-password"]

    presto_config_map_data = "\n".join(
        value
        for key, value in chart.get("ConfigMap", CONFIG_MAP)["data"].items()
        if key.startswith("presto-")
    )
    assert "clp.metadata-db-password" not in presto_config_map_data
    assert STATIC_CREDENTIALS["secret_access_key"] not in presto_config_map_data

    for deployment_name in [COORDINATOR, WORKER]:
        pod_spec = chart.get("Deployment", deployment_name)["spec"]["template"]["spec"]
        catalog_volume = next(v for v in pod_spec["volumes"] if "presto-catalog" == v["name"])
        assert CATALOG_SECRET == catalog_volume["secret"]["secretName"]


def test_presto_pods(default_auth_chart: RenderedChart) -> None:
    """
    Tests that both Presto pods install the connector from its installer image, tolerate a slow
    start, and roll when their config or catalog changes.
    """
    for deployment_name, install_path_env in [
        (COORDINATOR, "COORDINATOR_PLUGIN_INSTALL_PATH"),
        (WORKER, "WORKER_PLUGIN_INSTALL_PATH"),
    ]:
        installer = default_auth_chart.container(deployment_name, "install-clp-plugin")
        assert installer["image"].startswith("ghcr.io/y-scope/clp-plugin-presto-connector:")
        assert "@sha256:" in installer["image"]
        (install_env,) = installer["env"]
        assert install_path_env == install_env["name"]
        assert [install_env["value"]] == [m["mountPath"] for m in installer["volumeMounts"]]

        presto = default_auth_chart.container(deployment_name, deployment_name.removeprefix("clp-"))
        assert "@sha256:" in presto["image"]
        assert {"name": "presto-plugin", "mountPath": PLUGIN_DIR, "readOnly": True} in presto[
            "volumeMounts"
        ]
        startup_probe = presto["startupProbe"]
        assert MIN_STARTUP_PROBE_SECONDS <= (
            startup_probe["periodSeconds"] * startup_probe["failureThreshold"]
        )
        assert {"checksum/config", "checksum/presto-catalog"} <= set(
            _annotations(default_auth_chart, deployment_name)
        )

    service = default_auth_chart.get("Service", COORDINATOR)
    assert "ClusterIP" == service["spec"]["type"]

    config_map_data = default_auth_chart.get("ConfigMap", CONFIG_MAP)["data"]
    coordinator_config = parse_properties(
        config_map_data["presto-coordinator-config-config.properties"]
    )
    assert "true" == coordinator_config["use-connector-provided-serialization-codecs"]
    assert "-Xmx1536M" in config_map_data["presto-coordinator-config-jvm.config"].splitlines()
    worker_config = parse_properties(config_map_data["presto-worker-config-config.properties"])
    assert PLUGIN_DIR == worker_config["plugin.dir"]

    setup_configs = default_auth_chart.container(WORKER, "setup-configs")
    assert {
        "POD_IP": "status.podIP",
        "POD_NAME": "metadata.name",
    } == {env["name"]: env["valueFrom"]["fieldRef"]["fieldPath"] for env in setup_configs["env"]}


def test_presto_pods_roll_only_for_their_changes(default_auth_chart: RenderedChart) -> None:
    """Tests that each Presto pod's checksums change only when the entries it mounts change."""
    baseline = {name: _annotations(default_auth_chart, name) for name in [COORDINATOR, WORKER]}

    def changed_annotations(values: dict[str, Any]) -> set[tuple[str, str]]:
        chart = render_chart(values)
        return {
            (name, key)
            for name, annotations in baseline.items()
            for key, value in _annotations(chart, name).items()
            if value != annotations[key]
        }

    values = _default_auth_values()
    values["clpConfig"]["archive_output"]["storage"]["s3_config"]["bucket"] = "other-bucket"
    assert {(WORKER, "checksum/presto-catalog")} == changed_annotations(values)

    values = _default_auth_values()
    values["clpConfig"]["presto"]["worker"]["system_memory_gb"] = 4
    assert {(WORKER, "checksum/config")} == changed_annotations(values)

    values = _default_auth_values()
    values["credentials"] = {"database": {"password": "new-pass"}}
    assert {(COORDINATOR, "checksum/presto-catalog")} == changed_annotations(values)

    values = _default_auth_values()
    values["clpConfig"]["compression_worker"] = {"logging_level": "DEBUG"}
    assert set() == changed_annotations(values)


def _unescape_java_properties_value(value: str) -> str:
    escapes = {"\\": "\\", "n": "\n", "r": "\r", "t": "\t", "f": "\f", " ": " "}
    unescaped: list[str] = []
    chars = iter(value)
    for char in chars:
        unescaped.append(escapes[next(chars)] if "\\" == char else char)
    return "".join(unescaped)


def test_presto_coordinator_catalog_escapes_credentials() -> None:
    """Tests that the coordinator loads database credentials containing special characters."""
    special_value = " p\\a ss\tw\nord "
    values = _default_auth_values()
    values["credentials"] = {"database": {"password": special_value}}
    chart = render_chart(values)

    (escaped_value,) = [
        line.removeprefix("clp.metadata-db-password=")
        for line in chart.get("Secret", CATALOG_SECRET)["stringData"][
            "presto-coordinator-catalog-clp.properties"
        ].splitlines()
        if line.startswith("clp.metadata-db-password=")
    ]
    assert special_value == _unescape_java_properties_value(escaped_value)


@pytest.mark.parametrize(
    ("image", "expected_ref"),
    [
        ({"tag": "0.300"}, "registry.example/presto:0.300"),
        ({"tag": None, "digest": "sha256:abc"}, "registry.example/presto@sha256:abc"),
        ({"tag": "0.300", "digest": "sha256:abc"}, "registry.example/presto:0.300@sha256:abc"),
    ],
)
def test_image_refs(image: dict[str, Any], expected_ref: str) -> None:
    """Tests that images can be selected by tag, digest, or both."""
    values = _default_auth_values()
    values["image"] = {
        "prestoCoordinator": {"repository": "registry.example/presto", "digest": None, **image},
    }
    chart = render_chart(values)
    assert expected_ref == chart.container(COORDINATOR, "presto-coordinator")["image"]


@pytest.mark.parametrize(
    ("override", "error"),
    [
        (
            {"image": {"clpConnector": {"tag": None, "digest": None}}},
            'image.clpConnector requires "tag", "digest", or both',
        ),
        (
            {"clpConfig": {"presto": {"coordinator": {"jvm_heap_size": "1.5GB"}}}},
            "jvm_heap_size must be a JVM memory size",
        ),
        (
            {"clpConfig": {"archive_output": {"storage": {"s3_config": {"region_code": None}}}}},
            "Presto requires archive_output.storage.s3_config.region_code or endpoint_url.",
        ),
    ],
)
def test_presto_rejects_invalid_values(override: dict[str, Any], error: str) -> None:
    """Tests that invalid Presto values fail to render with an actionable error."""
    values = _merge(_default_auth_values(), override)
    with pytest.raises(HelmTemplateError, match=error):
        render_chart(values)


@pytest.mark.parametrize("auth_type", ["env_vars", "profile"])
def test_presto_rejects_unsupported_s3_auth(auth_type: str) -> None:
    """Tests that S3 authentication types that Presto workers can't use fail to render."""
    aws_authentication: dict[str, Any] = {"type": auth_type}
    if "profile" == auth_type:
        aws_authentication["profile"] = "default"
    values = _default_auth_values()
    values["clpConfig"]["archive_output"]["storage"]["s3_config"]["aws_authentication"] = (
        aws_authentication
    )
    with pytest.raises(HelmTemplateError, match=f'"default" or "credentials".*not "{auth_type}"'):
        render_chart(values)


def _merge(base: dict[str, Any], override: dict[str, Any]) -> dict[str, Any]:
    merged = copy.deepcopy(base)
    for key, value in override.items():
        if isinstance(value, dict) and isinstance(merged.get(key), dict):
            merged[key] = _merge(merged[key], value)
        else:
            merged[key] = value
    return merged
