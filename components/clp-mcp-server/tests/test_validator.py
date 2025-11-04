
from clp_mcp_server.validator import validate_query


def test_validate_query():
    valid_query = "Node AND ready"
    invalid_query = "Node became not ready"  # Invalid operator

    is_valid, error_msg = validate_query(valid_query)
    assert is_valid is True
    assert error_msg is None

    is_valid, error_msg = validate_query(invalid_query)
    assert is_valid is False
    assert error_msg is not None
