"""Fixtures for logging test lifecycle events."""

import logging
from collections.abc import Iterator

import pytest

logger = logging.getLogger(__name__)


@pytest.fixture(autouse=True)
def log_test_lifecycle(request: pytest.FixtureRequest) -> Iterator[None]:
    """
    Logs a message that identifies a test by name before it starts and after it finishes.

    :param request:
    """
    test_name = request.node.nodeid
    logger.info("Starting test: '%s'", test_name)
    yield
    logger.info("Test complete: '%s'", test_name)
