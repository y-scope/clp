"""Configuration helpers for CLP telemetry."""

import os

# Values accepted by both CLP_DISABLE_TELEMETRY and DO_NOT_TRACK to disable telemetry.
TELEMETRY_DISABLE_VALUES = {"1", "true", "yes", "y"}


def is_telemetry_disabled_by_env() -> bool:
    """:return: True if CLP_DISABLE_TELEMETRY or DO_NOT_TRACK disables telemetry."""
    disable_env_var = os.environ.get("CLP_DISABLE_TELEMETRY", "").strip().lower()
    dnt_env_var = os.environ.get("DO_NOT_TRACK", "").strip().lower()
    return disable_env_var in TELEMETRY_DISABLE_VALUES or dnt_env_var in TELEMETRY_DISABLE_VALUES