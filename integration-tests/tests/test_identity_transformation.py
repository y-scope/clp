import pytest

pytestmark = pytest.mark.binaries


@pytest.mark.clp
def test_always_pass_0() -> None:
    assert True


@pytest.mark.clp_s
def test_always_pass_1() -> None:
    assert True
