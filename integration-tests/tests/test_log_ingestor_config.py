"""Tests the distinction between host-published and internal ingestor addresses."""

from clp_py_utils.clp_config import ClpConfig, LOG_INGESTOR_COMPONENT_NAME, LogIngestor


def test_container_config_transforms_log_ingestor_address() -> None:
    """Internal clients use the service address without modifying host bind settings."""
    host_port = 13012
    original = ClpConfig(log_ingestor=LogIngestor(host="127.0.0.2", port=host_port))
    container = original.model_copy(deep=True)

    container.transform_for_container()

    assert container.log_ingestor is not None
    assert container.log_ingestor.host == LOG_INGESTOR_COMPONENT_NAME
    assert container.log_ingestor.port == LogIngestor.DEFAULT_PORT
    assert original.log_ingestor is not None
    assert original.log_ingestor.host == "127.0.0.2"
    assert original.log_ingestor.port == host_port


def test_disabled_log_ingestor_remains_disabled_in_container_config() -> None:
    """Transforming a deployment with no ingestor does not enable one."""
    config = ClpConfig(log_ingestor=None)

    config.transform_for_container()

    assert config.log_ingestor is None
