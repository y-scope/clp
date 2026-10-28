import pytest

from clp_py_utils.s3_utils import generate_s3_url


def test_default_endpoint_without_region() -> None:
    assert (
        generate_s3_url(None, None, "logs", "a/b.json") == "https://logs.s3.amazonaws.com/a/b.json"
    )


def test_default_endpoint_with_region() -> None:
    assert (
        generate_s3_url(None, "us-east-1", "logs", "a/b.json")
        == "https://logs.s3.us-east-1.amazonaws.com/a/b.json"
    )


def test_custom_endpoint_ignores_region() -> None:
    assert (
        generate_s3_url("http://minio:9000", "us-east-1", "logs", "a/b.json")
        == "http://minio:9000/logs/a/b.json"
    )


def test_custom_endpoint_with_s3_prefix_prepends_region() -> None:
    assert (
        generate_s3_url("https://s3.example.com", "us-east-1", "logs", "a/b.json")
        == "https://s3.us-east-1.example.com/logs/a/b.json"
    )


def test_unsupported_endpoint_url() -> None:
    with pytest.raises(ValueError):
        generate_s3_url("ftp://example.com", None, "logs", "a/b.json")
