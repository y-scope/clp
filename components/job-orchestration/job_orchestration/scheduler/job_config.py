from __future__ import annotations

from enum import auto
from typing import List, Literal, Optional, Tuple, Union

from clp_py_utils.clp_config import S3Config
from pydantic import BaseModel, field_validator
from strenum import LowercaseStrEnum


class InputType(LowercaseStrEnum):
    FS = auto()
    S3 = auto()


class PathsToCompress(BaseModel):
    file_paths: List[str]
    group_ids: List[int]
    st_sizes: List[int]
    empty_directories: Optional[List[str]] = None


class FsInputConfig(BaseModel):
    type: Literal[InputType.FS.value] = InputType.FS.value
    dataset: Optional[str] = None
    paths_to_compress: List[str]
    path_prefix_to_remove: str = None
    timestamp_key: Optional[str] = None
    unstructured: bool = False


class S3InputConfig(S3Config):
    type: Literal[InputType.S3.value] = InputType.S3.value
    keys: Optional[List[str]] = None
    dataset: Optional[str] = None
    timestamp_key: Optional[str] = None
    unstructured: bool = False

    @field_validator("keys")
    @classmethod
    def validate_keys(cls, value):
        if value is not None and len(value) == 0:
            raise ValueError("Keys cannot be an empty list")
        return value


class OutputConfig(BaseModel):
    tags: Optional[List[str]] = None
    target_archive_size: int
    target_dictionaries_size: int
    target_segment_size: int
    target_encoded_file_size: int
    compression_level: int


class ClpIoConfig(BaseModel):
    input: Union[FsInputConfig, S3InputConfig]
    output: OutputConfig


class AggregationConfig(BaseModel):
    job_id: Optional[int] = None
    reducer_host: Optional[str] = None
    reducer_port: Optional[int] = None
    do_count_aggregation: Optional[bool] = None
    count_by_time_bucket_size: Optional[int] = None  # Milliseconds


class QueryJobConfig(BaseModel):
    dataset: Optional[str] = None


class ExtractIrJobConfig(QueryJobConfig):
    orig_file_id: str
    msg_ix: int
    file_split_id: Optional[str] = None
    target_uncompressed_size: Optional[int] = None


class ExtractJsonJobConfig(QueryJobConfig):
    archive_id: str
    target_chunk_size: Optional[int] = None


class SearchJobConfig(QueryJobConfig):
    query_string: str
    max_num_results: int
    tags: Optional[List[str]] = None
    begin_timestamp: Optional[int] = None
    end_timestamp: Optional[int] = None
    ignore_case: bool = False
    path_filter: Optional[str] = None
    # Tuple of (host, port)
    network_address: Optional[Tuple[str, int]] = None
    aggregation_config: Optional[AggregationConfig] = None

    @field_validator("network_address")
    @classmethod
    def validate_network_address(cls, value):
        if value is not None and (value[1] < 1 or value[1] > 65535):
            raise ValueError("Port must be in the range [1, 65535]")

        return value
