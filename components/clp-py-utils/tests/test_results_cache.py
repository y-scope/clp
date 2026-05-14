"""Tests for ResultsCache TLS configuration and URI generation."""

import pytest
from pydantic import ValidationError

from clp_py_utils.clp_config import ResultsCache


def test_get_uri_default():
    """Default URI has no TLS parameters."""
    cache = ResultsCache()
    assert cache.get_uri() == "mongodb://localhost:27017/clp-query-results"


def test_get_uri_custom_host_port_db():
    """URI reflects custom host, port, and db_name."""
    cache = ResultsCache(host="docdb.example.com", port=27018, db_name="my-results")
    assert cache.get_uri() == "mongodb://docdb.example.com:27018/my-results"


def test_get_uri_tls_enabled():
    """Enabling TLS appends ?tls=true."""
    cache = ResultsCache(tls=True)
    assert cache.get_uri() == "mongodb://localhost:27017/clp-query-results?tls=true"


def test_get_uri_tls_with_ca_file():
    """TLS with CA file appends both params."""
    cache = ResultsCache(tls=True, tls_ca_file="/etc/ssl/certs/global-bundle.pem")
    assert (
        cache.get_uri()
        == "mongodb://localhost:27017/clp-query-results"
        "?tls=true&tlsCAFile=/etc/ssl/certs/global-bundle.pem"
    )


def test_tls_ca_file_without_tls_raises():
    """Setting tls_ca_file without tls=True raises a validation error."""
    with pytest.raises(ValidationError, match="tls_ca_file requires tls to be enabled"):
        ResultsCache(tls_ca_file="/etc/ssl/certs/ca.pem")


def test_tls_disabled_by_default():
    """TLS fields default to off/None."""
    cache = ResultsCache()
    assert cache.tls is False
    assert cache.tls_ca_file is None
