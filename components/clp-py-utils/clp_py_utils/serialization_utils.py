import pathlib

from strenum import StrEnum


def serialize_str_enum(member: StrEnum) -> str:
    """
    Serializes a `strenum.StrEnum` member to its underlying value.

    :param member:
    :return: The underlying string value of the enum member.
    """
    return member.value


def serialize_path(path: pathlib.Path) -> str:
    """
    Serializes a `pathlib.Path` to its string representation.

    :param path:
    :return: The string representation of the path.
    """
    return str(path)
