from __future__ import annotations

from enum import auto
from typing import Literal

from clp_py_utils.clp_config import S3Config
from pydantic import BaseModel, field_validator
from strenum import LowercaseStrEnum


class InputType(LowercaseStrEnum):
    FS = auto()
    S3 = auto()


class PathsToCompress(BaseModel):
    file_paths: list[str]
    group_ids: list[int]
    st_sizes: list[int]
    empty_directories: list[str] | None = None


class FsInputConfig(BaseModel):
    type: Literal[InputType.FS.value] = InputType.FS.value
    dataset: str | None = None
    paths_to_compress: list[str]
    path_prefix_to_remove: str = None
    timestamp_key: str | None = None
    unstructured: bool = False


class S3InputConfig(S3Config):
    type: Literal[InputType.S3.value] = InputType.S3.value
    keys: list[str] | None = None
    dataset: str | None = None
    timestamp_key: str | None = None
    unstructured: bool = False

    @field_validator("keys")
    @classmethod
    def validate_keys(cls, value):
        if value is not None and len(value) == 0:
            raise ValueError("Keys cannot be an empty list")
        return value


class OutputConfig(BaseModel):
    target_archive_size: int
    target_dictionaries_size: int
    target_segment_size: int
    target_encoded_file_size: int
    compression_level: int


class ClpIoConfig(BaseModel):
    input: FsInputConfig | S3InputConfig
    output: OutputConfig


class AggregationConfig(BaseModel):
    job_id: int | None = None
    reducer_host: str | None = None
    reducer_port: int | None = None
    do_count_aggregation: bool | None = None
    count_by_time_bucket_size: int | None = None  # Milliseconds


class QueryJobConfig(BaseModel):
    datasets: list[str] | None = None


class ExtractIrJobConfig(QueryJobConfig):
    orig_file_id: str
    msg_ix: int
    file_split_id: str | None = None
    target_uncompressed_size: int | None = None


class ExtractJsonJobConfig(QueryJobConfig):
    archive_id: str
    target_chunk_size: int | None = None


class SearchJobConfig(QueryJobConfig):
    query_string: str
    max_num_results: int
    begin_timestamp: int | None = None
    end_timestamp: int | None = None
    ignore_case: bool = False
    path_filter: str | None = None
    # Tuple of (host, port)
    network_address: tuple[str, int] | None = None
    aggregation_config: AggregationConfig | None = None
    write_to_file: bool = False

    @field_validator("network_address")
    @classmethod
    def validate_network_address(cls, value):
        if value is not None and (value[1] < 1 or value[1] > 65535):
            raise ValueError("Port must be in the range [1, 65535]")

        return value
