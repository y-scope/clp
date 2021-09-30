from pydantic import BaseModel
import typing

class PathsToCompress(BaseModel):
    file_paths: typing.List[str]
    group_ids: typing.List[int]
    st_sizes: typing.List[int]
    empty_directories: typing.List[str] = None


class InputConfig(BaseModel):
    type: str
    list_path: str
    path_prefix_to_remove: str = None


class OutputConfig(BaseModel):
    type: str
    target_archive_size: int
    target_dictionaries_size: int
    target_segment_size: int
    target_encoded_file_size: int
    storage_is_node_specific: bool


class ClpIoConfig(BaseModel):
    input: InputConfig
    output: OutputConfig
