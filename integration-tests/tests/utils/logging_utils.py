"""Utilities for logging during the test run."""

BLUE = "\033[34m"
BOLD = "\033[1m"
RESET = "\033[0m"


def construct_log_err_msg(err_msg: str) -> str:
    """
    Append a signal that directs readers to the test output log file.

    :param err_msg: The base error message onto which the signal will be appended.
    :return: An error message that directs readers to look in the test output log file.

    """
    return (
        err_msg
        + " Check the full test output log for more information (see test header for file path)."
    )
