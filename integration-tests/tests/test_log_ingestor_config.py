"""Tests the distinction between host-published and internal ingestor addresses."""

import pytest
from clp_package_utils.general import is_log_ingestor_enabled
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


@pytest.mark.parametrize(
    ("s3_input", "configured", "expected"),
    [(False, False, False), (False, True, False), (True, False, False), (True, True, True)],
)
def test_log_ingestor_enablement(s3_input: bool, configured: bool, expected: bool) -> None:
    """Only configured ingestors with S3 input are enabled; filesystem input stays valid."""
    config = ClpConfig.model_validate(
        {
            "logs_input": (
                {"type": "s3", "aws_authentication": {"type": "default"}}
                if s3_input
                else {"type": "fs"}
            ),
            "log_ingestor": {} if configured else None,
        }
    )

    assert is_log_ingestor_enabled(config) == expected
