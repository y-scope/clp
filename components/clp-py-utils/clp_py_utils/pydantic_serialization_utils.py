"""Pydantic serialization utilities."""

import pathlib
from typing import Annotated

from pydantic import PlainSerializer

StrEnumSerializer = PlainSerializer(lambda enum_value: enum_value.value)

PathStr = Annotated[pathlib.Path, PlainSerializer(lambda path_value: str(path_value))]
