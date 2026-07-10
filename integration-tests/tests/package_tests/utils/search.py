"""Classes to facilitate CLP package search testing."""

import logging
from datetime import datetime, timezone
from pathlib import Path

from tests.utils.classes import CmdArgs

logger = logging.getLogger(__name__)


def parse_timestamp_to_epoch_ms(timestamp: str, strptime_pattern: str) -> int:
    """
    Parses `timestamp` using `strptime_pattern` into an integer number of milliseconds since the
    UNIX epoch. A timestamp without an offset in the pattern is interpreted as UTC.

    :param timestamp:
    :param strptime_pattern:
    :raises ValueError: If `timestamp` doesn't match `strptime_pattern`.
    :return: The parsed timestamp as an integer number of milliseconds since the UNIX epoch.
    """
    parsed = datetime.strptime(timestamp, strptime_pattern)  # noqa: DTZ007
    if parsed.tzinfo is None:
        parsed = parsed.replace(tzinfo=timezone.utc)
    return round(parsed.timestamp() * 1000)


class SearchArgs(CmdArgs):
    """Command argument model for searching with the CLP package."""

    script_path: Path
    config: Path
    query: str
    raw: bool = True
    dataset: str | None = None
    file_path: Path | None = None
    ignore_case: bool = False
    count: bool = False
    count_by_time: int | None = None
    begin_ts: int | None = None
    end_ts: int | None = None

    @property
    def is_count_query(self) -> bool:
        """:return: `True` if this object represents a count-style search; else `False`."""
        return self.count or self.count_by_time is not None

    def to_cmd(self) -> list[str]:
        """Converts the model attributes to a command list."""
        cmd: list[str] = [
            str(self.script_path),
            "--config",
            str(self.config),
        ]

        if self.dataset:
            cmd.append("--dataset")
            cmd.append(self.dataset)
        if self.file_path:
            cmd.append("--file-path")
            cmd.append(str(self.file_path))
        if self.ignore_case:
            cmd.append("--ignore-case")
        if self.count:
            cmd.append("--count")
        if self.count_by_time is not None:
            cmd.append("--count-by-time")
            cmd.append(str(self.count_by_time))
        if self.begin_ts is not None:
            cmd.append("--begin-time")
            cmd.append(str(self.begin_ts))
        if self.end_ts is not None:
            cmd.append("--end-time")
            cmd.append(str(self.end_ts))
        if self.raw:
            cmd.append("--raw")

        cmd.append(self.query)

        return cmd
