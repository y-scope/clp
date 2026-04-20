# Integration test data

This directory (`clp/integration-tests/tests/data`) contains test datasets for use throughout the
integration test system.

## Directory structure

When augmenting or changing the contents of `clp/integration-tests/tests/data`, the following rules
must be observed:

1. Each dataset must be located in its own directory.
2. Each dataset directory must be a first-level directory within `clp/integration-tests/tests/data`,
   i.e., dataset directories cannot be nested.

## Dataset contents

When adding or changing a dataset directory within `clp/integration-tests/tests/data`, the following
rules must be observed:

1. Each dataset directory must contain a first-level subdirectory that holds all log files for the
   dataset.
2. The logs subdirectory may only contain log files. (These log files may be organized in any way
   within the logs subdirectory.)
3. Each dataset directory must contain a file called `metadata.json`.
4. The content of `metadata.json` must conform to the following schema:

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

   <dl>
   <dt><code>dataset_name</code></dt>
   <dd>The name of the dataset directory.</dd>
   <dt><code>unstructured</code></dt>
   <dd><code>True</code> if logs are unstructured, else <code>False</code>.</dd>
   <dt><code>timestamp_key</code></dt>
   <dd>The authoritative timestamp key, or `null` if there is no such key.</dd>
   <dt><code>begin_ts</code></dt>
   <dd>The earliest timestamp present in the dataset (ms).</dd>
   <dt><code>end_ts</code></dt>
   <dd>The latest timestamp present in the dataset (ms).</dd>
   <dt><code>logs_subdir</code></dt>
   <dd>The name of the subdirectory containing logs.</dd>
   <dt><code>file_names</code></dt>
   <dd>A list of the files within `logs_subdir`.</dd>
   <dt><code>single_match_wildcard_query</code></dt>
   <dd>A wildcard query which, when searched, will match a single log message in the dataset.</dd>
   </dl>

## Accessing datasets within the testing system

To access a dataset from within the test system, the following rules should be observed:

1. All datasets should have their own session-scoped fixture in
   `clp/integration-tests/tests/fixtures/datasets.py`. Test code should access this fixture to
   access the dataset.
2. Each session-scoped dataset fixture should be given the same name as the dataset directory name,
   and should conform to the following format:

   ```python
   @pytest.fixture(scope="session")
   def dataset_name(
       integration_test_path_config: IntegrationTestPathConfig,
   ) -> IntegrationTestDataset:
       """Returns an object corresponding to the `dataset_name` test dataset."""
       return IntegrationTestDataset(
           path_to_dataset_root=integration_test_path_config.test_data_path / "dataset_name",
       )
   ```

3. While it is possible to read the logs from a dataset directly (i.e., without using a 
   session-scoped dataset fixture), it is not recommended, as dataset metadata is needed in several
   verification processes throughout the testing system.
