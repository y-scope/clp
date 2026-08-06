# Integration test data

The `integration-tests/tests/data` directory contains sample datasets for use throughout the
integration test system. Each sample dataset is loaded into the test system through the
`SampleDataset` class.

## Directory structure

When augmenting or changing the contents of `integration-tests/tests/data`, the following rules
must be observed:

1. Each sample dataset must be located in its own directory.
2. Each sample dataset directory must be a direct child of `integration-tests/tests/data`; nested
   dataset directories are not permitted.

## Dataset contents

When adding or changing a sample dataset directory within `integration-tests/tests/data`, the
following rules must be observed:

1. Each sample dataset directory must contain a subdirectory that holds all log files for the
   dataset.
2. Each sample dataset directory must contain a file called `metadata.json`, which must conform to
   the following schema:

   ```json
   {
       "dataset_name": "str",
       "unstructured": bool,
       "timestamp_key": "str" | null,
       "timestamp_format": <timestamp-format> | null,
       "begin_ts": int,
       "end_ts": int,
       "logs_subdir": "str",
       "file_names": [
           "str",
           "str",
           ...
       ]
   }
   ```

   | Field | Description |
   | --- | --- |
   | `dataset_name` | The name of the sample dataset directory. |
   | `unstructured` | `True` if logs are unstructured, else `False`. |
   | `timestamp_key` | The authoritative timestamp key, or `null` if there is no such key. |
   | `timestamp_format` | Description of authoritative timestamp encoding (see below). |
   | `begin_ts` | The earliest timestamp present in the dataset (ms). |
   | `end_ts` | The latest timestamp present in the dataset (ms). |
   | `logs_subdir` | The name of the subdirectory containing logs. |
   | `file_names` | A list of the files within `logs_subdir`. |

   `timestamp_format` is a field tagged by by `kind` and containing `pattern` if applicable:

   ```json
   {"kind": "epoch_ms"}
   {"kind": "strptime", "pattern": "str"}
   ```

   | Field | Description |
   | --- | --- |
   | `kind` | `"epoch_ms"` for epoch ms timestamps, or `"strptime"` for formatted strings. |
   | `pattern` | (`strptime` only) The Python [`strptime`][strptime] pattern for the timestamp. |

3. For **structured** datasets: files in the dataset must be `.jsonl`, with only one log record per
   line.

## Time-range search verification

Search verification reproduces the package's `--begin-time`/`--end-time` filter by independently
determining each log event's timestamp. For a dataset to be usable in a time-range search test, the
following restrictions must be observed:

1. `begin_ts` must equal the earliest timestamp present in the dataset, and `end_ts` the latest.
2. `begin_ts` must be less than or equal to `end_ts`.
3. `timestamp_format` must not be `null`.
4. For **structured** datasets:
   * `timestamp_key` must not be `null`, and must be a *top-level* key. Nested/dotted keys are
     not supported.
   * Each log in the dataset must contain `timestamp_key`.
5. For **unstructured** datasets:
   * Every log line must begin with a single whitespace-delimited timestamp token written in the
     format described by `timestamp_format.pattern`.

## Accessing sample datasets within the testing system

To access a sample dataset from within the test system, the following rules should be observed:

1. All sample datasets should have their own session-scoped fixture in
   `integration-tests/tests/fixtures/sample_datasets.py`. Test code should access this fixture to
   access the sample dataset.
2. Each session-scoped fixture should be given the same name as the sample dataset directory name,
   and should conform to the following format:

   ```python
   @pytest.fixture(scope="session")
   def dataset_name(
       integration_test_path_config: IntegrationTestPathConfig,
   ) -> SampleDataset:
       """Returns an object corresponding to the `dataset_name` sample dataset."""
       return SampleDataset(
           dataset_root_dir=integration_test_path_config.test_data_dir / "dataset_name",
       )
   ```

Tests should use sample dataset fixtures instead of reading the logs directly, because many
verification flows rely on dataset metadata.

[strptime]: https://docs.python.org/3/library/datetime.html#strftime-and-strptime-format-codes
