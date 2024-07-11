from __future__ import annotations

import typing

from pydantic import BaseModel, validator


class PathsToCompress(BaseModel):
    file_paths: typing.List[str]
    group_ids: typing.List[int]
    st_sizes: typing.List[int]
    empty_directories: typing.List[str] = None


class InputConfig(BaseModel):
    paths_to_compress: typing.List[str]
    path_prefix_to_remove: str = None
    timestamp_key: typing.Optional[str] = None


class OutputConfig(BaseModel):
    tags: typing.Optional[typing.List[str]] = None
    target_archive_size: int
    target_dictionaries_size: int
    target_segment_size: int
    target_encoded_file_size: int


class ClpIoConfig(BaseModel):
    input: InputConfig
    output: OutputConfig


class AggregationConfig(BaseModel):
    job_id: typing.Optional[int] = None
    reducer_host: typing.Optional[str] = None
    reducer_port: typing.Optional[int] = None
    do_count_aggregation: typing.Optional[bool] = None
    count_by_time_bucket_size: typing.Optional[int] = None  # Milliseconds


class QueryJobConfig(BaseModel): ...


class ExtractIrJobConfig(QueryJobConfig):
    orig_file_id: str
    msg_ix: int
    file_split_id: typing.Optional[str] = None
    target_uncompressed_size: typing.Optional[int] = None


class SearchJobConfig(QueryJobConfig):
    query_string: str
    max_num_results: int
    tags: typing.Optional[typing.List[str]] = None
    begin_timestamp: typing.Optional[int] = None
    end_timestamp: typing.Optional[int] = None
    ignore_case: bool = False
    path_filter: typing.Optional[str] = None
    # Tuple of (host, port)
    network_address: typing.Optional[typing.Tuple[str, int]] = None
    aggregation_config: typing.Optional[AggregationConfig] = None

    @validator("network_address")
    def validate_network_address(cls, field):
        if field is not None and (field[1] < 1 or field[1] > 65535):
            raise ValueError("Port must be in the range [1, 65535]")

        return field
