"""Integration tests for reading multiple clp-s archives in one CLI invocation."""

import json
from pathlib import Path
from typing import Final

import pytest

from tests.binary_tests.clp_s_utils import compress, CompressOptions, extract, search
from tests.utils.config import ClpCorePathConfig

pytestmark = pytest.mark.core

_NEEDLE_IDX: Final = 42


@pytest.mark.clp_s
@pytest.mark.parametrize("single_file_archive", [False, True], ids=["multifile", "sfa"])
def test_clp_s_reads_all_split_archives(
    single_file_archive: bool,
    clp_core_path_config: ClpCorePathConfig,
    tmp_path: Path,
) -> None:
    """
    Verifies that search and extraction include records from every split archive.

    :param single_file_archive: Whether to compress each archive as a single file.
    :param clp_core_path_config:
    :param tmp_path:
    """
    events = [
        {
            "idx": idx,
            "message": (
                "message containing a needle" if idx == _NEEDLE_IDX else "a common message"
            ),
        }
        for idx in range(100)
    ]
    input_path = tmp_path / "input.jsonl"
    input_path.write_text("".join(json.dumps(event) + "\n" for event in events))

    archives_dir = tmp_path / "archives"
    compress(
        clp_core_path_config,
        archives_dir,
        input_path,
        options=CompressOptions(
            single_file_archive=single_file_archive,
            target_encoded_size=256,
        ),
    )
    assert len(list(archives_dir.iterdir())) > 1

    results = search(clp_core_path_config, archives_dir, "idx: *")
    assert sorted(result["idx"] for result in results) == list(range(len(events)))
    results = search(clp_core_path_config, archives_dir, 'message: "*needle*"')
    assert [result["idx"] for result in results] == [_NEEDLE_IDX]

    output_dir = tmp_path / "output"
    extract(clp_core_path_config, archives_dir, output_dir)
    extracted_events = [
        json.loads(line) for line in (output_dir / "original").read_text().splitlines()
    ]
    assert sorted(extracted_events, key=lambda event: event["idx"]) == events
