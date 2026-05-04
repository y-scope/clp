"""Utilities for logging during the test run."""

import logging

from tests.utils.classes import ExternalAction

logger = logging.getLogger(__name__)


def format_action_failure_msg(reason: str, *actions: ExternalAction) -> str:
    """
    Formats a failure message that indicates where to find the subprocess log(s) relevant to the
    failure. This function associates a single failure with one or more external actions.

    :param reason:
    :param actions:
    :return: The failure message.
    """
    action_log_paths = [str(action.log_file_path) for action in actions]
    return f"{reason} See relevant subprocess log(s) at: {action_log_paths}"
