from __future__ import annotations

import spider_py


def int8_list_to_utf8_str(byte_list: list[spider_py.Int8]) -> str:
    """
    Converts a list of `spider_py.Int8` values to a UTF-8 encoded string.

    :param byte_list:
    :return: Decoded UTF-8 string constructed from the input byte list.
    """
    return bytes(int(byte) for byte in byte_list).decode("utf-8")


def utf8_str_to_int8_list(utf8_str: str) -> list[spider_py.Int8]:
    """
    Converts a UTF-8 encoded string to a list of `spider_py.Int8` values.

    :param utf8_str:
    :return: A list of `spider_py.Int8` values representing the input string.
    """
    return [spider_py.Int8(byte) for byte in utf8_str.encode("utf-8")]
