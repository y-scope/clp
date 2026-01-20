#!/usr/bin/env python3

import io
import json
import random
import tempfile
import time
from datetime import datetime
from pathlib import Path

from yscope_clp_core import open_archive, search_archive, KqlQuery

import os
tempfile.tempdir = os.path.expandvars("/tmp/$USER")
Path(tempfile.tempdir).mkdir(parents=True, exist_ok=True)

def generate_json_line() -> str:
    rand = random.Random()
    log_levels = ("INFO", "WARN", "ERROR", "DEBUG")
    now = datetime.now()
    timestamp = f"{now:%Y-%m-%d %H:%M:%S}:{now.microsecond // 1000:03d}"
    log = {
        "timestamp": timestamp,
        "id": rand.randint(0, 9),
        "status": "failed" if rand.random() < 0.5 else "success",
        "level": rand.choice(log_levels),
    }
    return json.dumps(log) + "\n"

def main0():
    in_memory_input_fd = io.StringIO()
    local_input_file = Path("input_log.jsonl")

    with local_input_file.open("w") as local_input_fd:
        for _ in range(10000):
            line = generate_json_line()
            in_memory_input_fd.write(line)
            local_input_fd.write(line)

    # Build an archive from the local file into an in memory buffer
    in_memory_archive_fd = io.BytesIO()
    with open_archive(in_memory_archive_fd, mode="w", compression_level=5) as f:
        f.add(local_input_file)

    # Build a local archive directly from the in memory input
    local_archive = Path("archive")
    with open_archive(local_archive, mode="w", compression_level=12) as f:
        in_memory_input_fd.seek(0)
        f.add(in_memory_input_fd)

    # Representative KQL queries to validate search equivalence
    queries = [
        'status: failed AND level: ERROR AND id >= 5',
        '(status: failed OR level: WARN) AND id < 3',
        'status: success AND (level: INFO OR level: DEBUG)',
        '(status: failed AND level: ERROR) OR (status: success AND level: WARN)',
        'NOT (status: success AND level: INFO)',
        'status: failed AND NOT (level: DEBUG OR id: 0)',
        '(level: ERROR OR level: WARN) AND NOT status: success',
        'status: f* AND level: E*',
        '(status: success OR status: failed) AND level: *',
        'id >= 3 AND id <= 7 AND NOT level: DEBUG',
        '((status: failed AND id > 5) OR (level: WARN AND id < 2)) AND NOT level: INFO',
        'status: failed AND (level: ERROR OR (level: WARN AND id >= 4))',
    ]

    # For each query, ensure search results match exactly
    for q in queries:
        kql_query = KqlQuery(q)
        in_memory_archive_fd.seek(0)
        with (
            search_archive(local_archive, query=kql_query) as local_search,
            search_archive(in_memory_archive_fd, query=kql_query) as in_memory_search,
        ):
            for log_event in local_search:
                assert (
                    log_event.get_kv_pairs()
                    == in_memory_search.get_next_log_event().get_kv_pairs()
                )
            try:
                in_memory_search.get_next_log_event()
                raise AssertionError("Expected no more results")
            except StopIteration:
                pass

    # Validate full decompression equivalence between local and in memory archives
    local_decomp = open_archive(local_archive, mode="r")
    in_memory_archive_fd.seek(0)
    in_memory_decomp = open_archive(in_memory_archive_fd, mode="r")

    for log_event in local_decomp:
        assert(
            log_event.get_kv_pairs()
            == in_memory_decomp.get_next_log_event().get_kv_pairs()
        )

    # Ensure no trailing events remain
    try:
        in_memory_decomp.get_next_log_event()
        raise AssertionError("Expected no more results")
    except StopIteration:
        pass

    local_decomp.close()
    in_memory_decomp.close()

if __name__ == "__main__":
    main0()
