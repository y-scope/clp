"""Timestamp-format handling for CLP integration tests."""

from __future__ import annotations

import datetime
from typing import Any, Literal

from pydantic import BaseModel, model_validator
from typing_extensions import Self


class TimestampFormat(BaseModel):
    """
    Indicates the format of a timestamp in a dataset. An `epoch_ms` timestamp is an integer number
    of milliseconds since the UNIX epoch; a `strptime` timestamp is a string parseable by the
    `datetime.strptime` `pattern`.
    """

    type: Literal["epoch_ms", "strptime"]
    pattern: str | None = None

    @model_validator(mode="after")
    def _validate_pattern(self) -> Self:
        """
        Ensures `pattern` is present iff the format is `strptime`.

        :raises ValueError: If a `strptime` format omits `pattern`, or an `epoch_ms` format defines
            one.
        :return: The validated model.
        """
        if self.type == "strptime" and self.pattern is None:
            err_msg = "A 'strptime' timestamp format requires a 'pattern'."
            raise ValueError(err_msg)
        if self.type == "epoch_ms" and self.pattern is not None:
            err_msg = "An 'epoch_ms' timestamp format must not define a 'pattern'."
            raise ValueError(err_msg)
        return self

    def to_epoch_ms(self, raw_timestamp: Any) -> int:
        """
        Converts `raw_timestamp` into an integer number of milliseconds since the UNIX epoch,
        according to this format. A `strptime` timestamp without an offset in the pattern is
        interpreted as UTC.

        :param raw_timestamp:
        :raises TypeError:
        :raises ValueError:
        :return: The timestamp as an integer number of milliseconds since the UNIX epoch.
        """
        if self.type == "epoch_ms":
            if not isinstance(raw_timestamp, int):
                err_msg = f"Expected an integer epoch-ms timestamp, but got {raw_timestamp!r}."
                raise TypeError(err_msg)
            return raw_timestamp

        if not isinstance(raw_timestamp, str):
            err_msg = f"Expected a string timestamp, but got {raw_timestamp!r}."
            raise TypeError(err_msg)
        if self.pattern is None:
            err_msg = "Internal error: 'strptime' format missing 'pattern'."
            raise ValueError(err_msg)
        parsed = datetime.datetime.strptime(raw_timestamp, self.pattern)  # noqa: DTZ007
        if parsed.tzinfo is None:
            parsed = parsed.replace(tzinfo=datetime.timezone.utc)
        return round(parsed.timestamp() * 1000)
