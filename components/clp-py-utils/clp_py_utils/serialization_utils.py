import pathlib
from strenum import StrEnum


def serialize_enum(enum_value: StrEnum) -> str:
    """
    Serializes a StrEnum to its underlying value.

    :param enum_value: A StrEnum instance.
    :return: The underlying string value of the StrEnum.
    """
    return enum_value.value


def serialize_path(path: pathlib.Path) -> str:
    """
    Serializes a pathlib.Path to its string representation.

    :param path: A pathlib.Path instance.
    :return: The string representation of the path.
    """
    return str(path)
