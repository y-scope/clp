#!/usr/bin/env -S uv run --script
# /// script
# dependencies = []
# ///
"""Script to get an unused port and print the port number to stdout."""

import socket
import sys


def get_free_port() -> int:
    """
    Finds an unused port on localhost.

    :return: An unused port number.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("", 0))
        return s.getsockname()[1]


def main() -> int:
    """Main."""
    port = get_free_port()
    # ruff: noqa: T201
    print(port)
    return 0


if __name__ == "__main__":
    sys.exit(main())
