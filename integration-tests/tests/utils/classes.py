"""Classes used in CLP integration tests."""

from __future__ import annotations

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

        # Create `downloaded_logs_dir`.
        self.downloaded_logs_dir.mkdir(parents=True, exist_ok=True)

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

    @property
    def downloaded_logs_dir(self) -> Path:
        """:return: The absolute path to the directory where downloaded logs are stored."""
        return self.test_cache_dir / "downloaded_logs"

    def _static_paths(self) -> list[Path]:
        """:return: List of paths that must exist on disk at construction time."""
        return [self.test_data_dir]


class SampleDatasetMetadata(BaseModel):
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
class SampleDataset:
    """Path layout and metadata storage for a sample dataset."""

    #: Absolute path to the dataset root directory.
    dataset_root_dir: Path

    #: Pydantic model of metadata describing the dataset.
    metadata: SampleDatasetMetadata = field(init=False)

    #: The name of the dataset (for logging purposes).
    dataset_name: str = field(init=False)

    def __post_init__(self) -> None:
        """Validate data members and load metadata."""
        validate_dir_exists(self.dataset_root_dir)

        # Load metadata.
        validate_file_exists(self.metadata_file_path)
        raw_metadata = self.metadata_file_path.read_text()
        self.metadata = SampleDatasetMetadata.model_validate_json(raw_metadata)

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
    """
    Base class for external subprocesses executed during an integration test. Not intended for
    direct instantiation; users should instead instantiate `NonClpAction` or `ClpAction` derived
    classes as appropriate.
    """

    #: Command to pass to `subprocess.run()`.
    cmd: list[str]

    #: The completed process returned from `subprocess.run()`.
    completed_proc: subprocess.CompletedProcess[str] = field(init=False)

    #: Path to the file where this action's subprocess output was logged.
    log_file_path: Path = field(init=False)

    def __post_init__(self) -> None:
        """Execute the external action and log output."""
        if not self.cmd:
            pytest.fail(f"Cannot create '{type(self).__name__}' object: `cmd` list is empty.")

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


@dataclass
class NonClpAction(ExternalAction):
    """
    External action for a non-CLP subprocess (e.g., `grep`, `docker`, etc.). The testing system
    assumes that all non-CLP programs are functioning properly, and so `check_returncode()` raises
    `RuntimeError` if this is not the case.
    """

    def format_failure_msg(
        self,
        reason: str,
        related_action: ExternalAction | None = None,
    ) -> str:
        """
        Format a failure message that includes `reason` and the path to this action's log file.
        When this action was being used to verify another action, `related_action` should be passed
        in so that the path to its log file is also included in the failure message.

        :param reason: A description of the failure.
        :param related_action: An action whose verification depends on this action.
        :return: The formatted failure message.
        """
        msg = f"{reason} See subprocess log at: '{self.log_file_path}'."
        if related_action is not None:
            related_exe = Path(related_action.cmd[0]).name
            msg += (
                f" This failure occurred during the verification of '{related_exe}';"
                f" see related subprocess log at: '{related_action.log_file_path}'."
            )
        return msg

    def check_returncode(
        self,
        good_returncodes: tuple[int, ...] = (0,),
        related_action: ExternalAction | None = None,
    ) -> None:
        """
        :param good_returncodes:
        :param related_action: Another action whose verification depends on this action. Its log
            path is included in the failure message to aid debugging.
        :raise RuntimeError: if `completed_proc.returncode` is not in `good_returncodes`.
        """
        if self.completed_proc.returncode in good_returncodes:
            return
        reason = (
            f"The '{Path(self.cmd[0]).name}' subprocess returned a bad return code"
            f" ({self.completed_proc.returncode})."
        )
        err_msg = self.format_failure_msg(reason, related_action=related_action)
        logger.error(err_msg)
        raise RuntimeError(err_msg)


@dataclass
class ClpAction(ExternalAction):
    """
    External action for a CLP subprocess (e.g., `compress.sh`). The testing system does not assume
    that CLP is functioning properly, and so `verify_returncode()` returns a `ClpVerificationResult`
    object which should be processed at the callsite. Instances should be constructed via
    `from_args` or `from_cmd`.
    """

    #: Optional structured arguments. Not used by `ClpAction` itself; available for verification.
    args: CmdArgs | None = None

    @classmethod
    def from_cmd(cls, cmd: list[str]) -> Self:
        """:return: A `ClpAction` for the given raw `cmd`, with no associated `args`."""
        return cls(cmd=cmd)

    @classmethod
    def from_args(cls, args: CmdArgs) -> Self:
        """:return: A `ClpAction` whose `cmd` is derived from `args.to_cmd()`."""
        return cls(cmd=args.to_cmd(), args=args)

    def __post_init__(self) -> None:
        """
        Validate `args`/`cmd` agreement when both are provided during construction. Then execute the
        action.
        """
        if self.args is not None and self.cmd != self.args.to_cmd():
            pytest.fail("Cannot create `ClpAction` object: `cmd` does not match `args.to_cmd()`.")
        super().__post_init__()

    def format_failure_msg(
        self,
        reason: str,
        supporting_action: ClpAction | None = None,
    ) -> str:
        """
        Format a failure message that includes `reason` and the path to this action's log file.
        When this action's verification has failed as a direct result of some other `ClpAction`,
        this other action should be passed into `supporting_action` so that the path to its log file
        can be included in the failure message.

        :param reason: A description of the failure.
        :param supporting_action: A previous action that caused this failure.
        :return: The formatted failure message.
        """
        msg = f"{reason} See subprocess log at: '{self.log_file_path}'."
        if supporting_action is not None:
            supporting_exe = Path(supporting_action.cmd[0]).name
            msg += (
                f" See supporting subprocess ({supporting_exe}) log at:"
                f" '{supporting_action.log_file_path}'."
            )
        return msg

    def verify_returncode(
        self,
        good_returncodes: tuple[int, ...] = (0,),
    ) -> ClpVerificationResult:
        """
        :param good_returncodes:
        :return: A successful `ClpVerificationResult` if `completed_proc.returncode` is in
            `good_returncodes`; otherwise a failed `ClpVerificationResult` with a message describing
            the bad return code.
        """
        if self.completed_proc.returncode in good_returncodes:
            return ClpVerificationResult.ok()

        reason = (
            f"The '{Path(self.cmd[0]).name}' subprocess returned a bad return code"
            f" ({self.completed_proc.returncode})."
        )
        return ClpVerificationResult.fail(self, reason)


@dataclass(frozen=True)
class ClpVerificationResult:
    """Outcome returned from functions that verify CLP functionality."""

    #: Whether or not the verification was successful.
    success: bool

    #: Message describing the failure, if the verification failed.
    failure_message: str = ""

    def __bool__(self) -> bool:
        """Makes class truthy."""
        return self.success

    @classmethod
    def ok(cls) -> Self:
        """:return: A successful `ClpVerificationResult`."""
        return cls(success=True)

    @classmethod
    def fail(
        cls,
        action: ClpAction,
        reason: str,
        supporting_action: ClpAction | None = None,
    ) -> Self:
        """:return: A failed `ClpVerificationResult` carrying `failure_message`."""
        failure_message = action.format_failure_msg(reason, supporting_action=supporting_action)
        logger.warning(failure_message)
        return cls(success=False, failure_message=failure_message)
