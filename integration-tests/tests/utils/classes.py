"""Classes used in CLP integration tests."""

import datetime
import logging
import subprocess
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from pathlib import Path

import pytest
from pydantic import BaseModel
from typing_extensions import Self

from tests.conftest import get_test_log_dir
from tests.utils.utils import validate_dir_exists, validate_file_exists

DEFAULT_CMD_TIMEOUT_SECONDS = 120.0

logger = logging.getLogger(__name__)


@dataclass
class IntegrationTestPathConfig:
    """Path configuration for CLP integration tests."""

    #: Default CLP build directory.
    clp_build_dir: Path

    #: Default integration test project root directory.
    integration_tests_project_root: Path

    def __post_init__(self) -> None:
        """Create and validate directories."""
        # Validate that init directories exist.
        validate_dir_exists(self.clp_build_dir)
        validate_dir_exists(self.integration_tests_project_root)

        # Validate all static paths.
        for path in self._static_paths():
            if not path.exists():
                pytest.fail(f"Expected path does not exist: '{path}'")

        # Create `test_cache_dir`.
        self.test_cache_dir.mkdir(parents=True, exist_ok=True)

        # Create `test_log_dir`.
        self.test_log_dir.mkdir(parents=True, exist_ok=True)

    @property
    def test_cache_dir(self) -> Path:
        """:return: The absolute path to the integration test cache directory."""
        return self.clp_build_dir / "integration_tests"

    @property
    def test_data_dir(self) -> Path:
        """:return: The absolute path to the sample dataset directory."""
        return self.integration_tests_project_root / "tests" / "data"

    @property
    def test_log_dir(self) -> Path:
        """:return: The absolute path to the integration test log directory."""
        return self.test_cache_dir / "test_logs"

    def _static_paths(self) -> list[Path]:
        """:return: List of paths that must exist on disk at construction time."""
        return [self.test_data_dir]


class IntegrationTestDatasetMetadata(BaseModel):
    """
    Metadata for a sample dataset. All `<dataset_name>/metadata.json` files must conform to this
    schema.
    """

    dataset_name: str
    unstructured: bool
    timestamp_key: str | None
    begin_ts: int
    end_ts: int
    logs_subdir: str
    file_names: list[str]
    single_match_wildcard_query: str


@dataclass
class IntegrationTestDataset:
    """Path layout and metadata storage for a sample dataset."""

    #: Absolute path to the dataset root directory.
    dataset_root_dir: Path

    #: Pydantic model of metadata describing the dataset.
    metadata: IntegrationTestDatasetMetadata = field(init=False)

    #: The name of the dataset (for logging purposes).
    dataset_name: str = field(init=False)

    def __post_init__(self) -> None:
        """Validate data members and load metadata."""
        validate_dir_exists(self.dataset_root_dir)

        # Load metadata.
        validate_file_exists(self.metadata_file_path)
        raw_metadata = self.metadata_file_path.read_text()
        self.metadata = IntegrationTestDatasetMetadata.model_validate_json(raw_metadata)

        # Set dataset name from metadata.
        self.dataset_name = self.metadata.dataset_name

        # Validate metadata properties.
        validate_dir_exists(self.logs_path)

        if self.metadata.begin_ts > self.metadata.end_ts:
            err_msg = (
                f"Dataset metadata failure: `begin_ts` '{self.metadata.begin_ts}' is larger than"
                f" `end_ts` '{self.metadata.end_ts}'"
            )
            raise ValueError(err_msg)

        for file_path in self.metadata.file_names:
            file_path_abs = self.logs_path / file_path
            validate_file_exists(file_path_abs)

    @property
    def metadata_file_path(self) -> Path:
        """:return: The absolute path to the file containing metadata for the dataset."""
        return self.dataset_root_dir / "metadata.json"

    @property
    def logs_path(self) -> Path:
        """:return: The absolute path to the logs directory."""
        return self.dataset_root_dir / self.metadata.logs_subdir


class CmdArgs(BaseModel, ABC):
    """Abstract base class for all CLP command argument models."""

    @abstractmethod
    def to_cmd(self) -> list[str]:
        """:return: list of command arguments constructed from this instance's data members."""


@dataclass
class ExternalAction:
    """Metadata for an external action executed during an integration test."""

    #: Command to pass to `subprocess.run()`.
    cmd: list[str]

    #: Optional structured arguments for verification purposes. Not used by `ExternalAction` itself.
    args: CmdArgs | None = None

    #: The completed process returned from `subprocess.run()`.
    completed_proc: subprocess.CompletedProcess[str] = field(init=False)

    #: Path to the file where this action's subprocess output was logged.
    log_file_path: Path = field(init=False)

    def __post_init__(self) -> None:
        """Execute the external action and log output."""
        if not self.cmd:
            pytest.fail("Cannot create `ExternalAction` object: `cmd` list is empty.")
        self.completed_proc = self._run_subprocess()
        self._log_action_summary_to_file()

    def get_output(self) -> str:
        """:return: The combined stdout and stderr from the completed subprocess."""
        return self.completed_proc.stdout + self.completed_proc.stderr

    def _run_subprocess(self) -> subprocess.CompletedProcess[str]:
        """
        Passes `self.cmd` to `subprocess.run()` with preset parameters:
            capture_output=True:                    Output will be logged and analysed later.
            timeout=DEFAULT_CMD_TIMEOUT_SECONDS:
            check=False:                            Error will be handled during verification.
            text=True:                              Output should be str for analysis purposes.
        """
        exe_name = Path(self.cmd[0]).name
        log_msg = f"Running '{exe_name}' subprocess. Command: {self.cmd}"
        logger.info(log_msg)

        try:
            return subprocess.run(
                self.cmd,
                capture_output=True,
                timeout=DEFAULT_CMD_TIMEOUT_SECONDS,
                check=False,
                text=True,
            )
        except subprocess.TimeoutExpired:
            pytest.fail(f"Subprocess '{exe_name}' timed out after {DEFAULT_CMD_TIMEOUT_SECONDS}s.")
        except OSError as e:
            pytest.fail(f"Subprocess '{exe_name}' failed to start: {e}")

    def _log_action_summary_to_file(self) -> None:
        """Logs a summary of the external action execution to a unique file."""
        now = datetime.datetime.now()  # noqa: DTZ005
        test_run_id = now.strftime("%Y-%m-%d-%H-%M-%S-%f")[:-3]
        self.log_file_path = (
            get_test_log_dir() / "subprocess_output" / f"{Path(self.cmd[0]).name}_{test_run_id}.log"
        )

        completed_proc = self.completed_proc
        stdout_content = completed_proc.stdout or "(empty)"
        stderr_content = completed_proc.stderr or "(empty)"

        if not stdout_content.endswith("\n"):
            stdout_content += "\n"
        if not stderr_content.endswith("\n"):
            stderr_content += "\n"

        sep = "-" * 32
        summary_lines = [
            "SUBPROCESS RUN SUMMARY\n",
            f"{sep}\n",
            f"Timestamp at completion : {now.strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]}\n",
            f"Command                 : {completed_proc.args}\n",
            f"Return Code             : {completed_proc.returncode}\n",
            "\n\n",
            "captured stdout\n",
            f"{sep}\n",
            stdout_content,
            "\n",
            "\n\n",
            "captured stderr\n",
            f"{sep}\n",
            stderr_content,
            "\n",
        ]

        self.log_file_path.parent.mkdir(parents=True, exist_ok=True)
        with self.log_file_path.open("w", encoding="utf-8") as log_file:
            log_file.writelines(summary_lines)

        log_msg = (
            f"Subprocess returned. stdout and stderr written to log file: '{self.log_file_path}'"
        )
        logger.info(log_msg)


@dataclass(frozen=True)
class VerificationResult:
    """Outcome from a verification function."""

    #: Whether or not the verification was successful.
    success: bool

    #: Message describing the failure, if the verification failed.
    failure_message: str = ""

    def __bool__(self) -> bool:
        """Makes class truthy."""
        return self.success

    @classmethod
    def ok(cls) -> Self:
        """:return: A successful `VerificationResult`."""
        return cls(success=True)

    @classmethod
    def fail(cls, failure_message: str) -> Self:
        """:return: A failed `VerificationResult` carrying `failure_message`."""
        return cls(success=False, failure_message=failure_message)
