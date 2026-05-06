# Integration test data

The `integration-tests/tests/data` directory contains test datasets for use throughout the
integration test system.

## Directory structure

When augmenting or changing the contents of `integration-tests/tests/data`, the following rules
must be observed:

1. Each dataset must be located in its own directory.
2. Each dataset directory must be a direct child of `integration-tests/tests/data`; nested dataset
   directories are not permitted.

## Dataset contents

When adding or changing a dataset directory within `integration-tests/tests/data`, the following
rules must be observed:

1. Each dataset directory must contain a subdirectory that holds all log files for the dataset.
2. Each dataset directory must contain a file called `metadata.json`, which must conform to the
   following schema:

   ```json
   {
       "dataset_name": "str",
       "unstructured": bool,
       "timestamp_key": "str" | null,
       "begin_ts": int,
       "end_ts": int,
       "logs_subdir": "str",
       "file_names": [
           "str",
           "str",
           ...
       ],
       "single_match_wildcard_query": "str"
   }
   ```

   | Field | Description |
   | --- | --- |
   | `dataset_name` | The name of the dataset directory. |
   | `unstructured` | `True` if logs are unstructured, else `False`. |
   | `timestamp_key` | The authoritative timestamp key, or `null` if there is no such key. |
   | `begin_ts` | The earliest timestamp present in the dataset (ms). |
   | `end_ts` | The latest timestamp present in the dataset (ms). |
   | `logs_subdir` | The name of the subdirectory containing logs. |
   | `file_names` | A list of the files within `logs_subdir`. |
   | `single_match_wildcard_query` | A wildcard query that matches exactly one log message in the dataset. |

## Accessing datasets within the testing system

To access a dataset from within the test system, the following rules should be observed:

1. All datasets should have their own session-scoped fixture in
   `integration-tests/tests/fixtures/datasets.py`. Test code should access this fixture to
   access the dataset.
2. Each session-scoped dataset fixture should be given the same name as the dataset directory name,
   and should conform to the following format:

   ```python
   @pytest.fixture(scope="session")
   def dataset_name(
       integration_test_path_config: IntegrationTestPathConfig,
   ) -> SampleDataset:
       """Returns an object corresponding to the `dataset_name` test dataset."""
       return SampleDataset(
           dataset_root_dir=integration_test_path_config.test_data_dir / "dataset_name",
       )
   ```

Tests should use dataset fixtures instead of reading the logs directly, because many verification
flows rely on dataset metadata.
