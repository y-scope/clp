from pydantic import BaseModel, validator

from clp_py_utils.pretty_size import pretty_size


# Limited set of configurations operation found in clp_config.py
class ArchiveOutput(BaseModel):
    target_archive_size: int
    target_dictionaries_size: int
    target_encoded_file_size: int
    target_segment_size: int

    @validator('target_archive_size')
    def validate_target_archive_size(cls, field):
        if field <= 0:
            raise ValueError('target_archive_size parameter must be greater than 0')
        return field

    @validator('target_dictionaries_size')
    def validate_target_dictionaries_size(cls, field):
        if field <= 0:
            raise ValueError('target_dictionaries_size parameter must be greater than 0')
        return field

    @validator('target_encoded_file_size')
    def validate_target_encoded_file_size(cls, field):
        if field <= 0:
            raise ValueError('target_encoded_file_size parameter must be greater than 0')
        return field

    @validator('target_segment_size')
    def validate_target_segment_size(cls, field):
        if field <= 0:
            raise ValueError('target_segment_size parameter must be greater than 0')
        return field


class CLPPackageConfig(BaseModel):
    cluster_name: str
    archive_output: ArchiveOutput

    def generate_package_config_file_content_with_comments(self):
        file_content = [
            f'cluster_name: {self.cluster_name}',
            f'',
            f'archive_output:',
            f'  # How much data CLP should try to compress into each archive',
            f'  target_archive_size: {str(self.archive_output.target_archive_size)}   # {pretty_size(self.archive_output.target_archive_size)}',
            f'',
            f'  # How large the dictionaries should be allowed to get before the archive is closed and a new one is created',
            f'  target_dictionaries_size: {str(self.archive_output.target_dictionaries_size)}   # {pretty_size(self.archive_output.target_dictionaries_size)}',
            f'',
            f'  # How large each encoded file should be before being split into a new encoded file',
            f'  target_encoded_file_size: {str(self.archive_output.target_encoded_file_size)}   # {pretty_size(self.archive_output.target_encoded_file_size)}',
            f'',
            f'  # How much data CLP should try to fit into each segment within an archive',
            f'  target_segment_size: {str(self.archive_output.target_segment_size)}   # {pretty_size(self.archive_output.target_segment_size)}',
            f''
        ]
        return '\n'.join(file_content)
