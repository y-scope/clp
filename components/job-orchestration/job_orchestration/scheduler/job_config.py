from __future__ import annotations

import typing

from pydantic import BaseModel


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
    count_by_time_bucket_size: typing.Optional[int] = None  // Milliseconds


class SearchConfig(BaseModel):
    query_string: str
    max_num_results: int
    tags: typing.Optional[typing.List[str]] = None
    begin_timestamp: typing.Optional[int] = None
    end_timestamp: typing.Optional[int] = None
    ignore_case: bool = False
    path_filter: typing.Optional[str] = None
    aggregation_config: typing.Optional[AggregationConfig] = None
