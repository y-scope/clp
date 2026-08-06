"""Command argument model for the CLP package archive-manager."""

from pathlib import Path

from strenum import StrEnum

from tests.package_tests.classes import ClpPackage
from tests.utils.classes import (
    ClpAction,
    CmdArgs,
    SampleDataset,
)


class ClpPackageArchiveManagerCommand(StrEnum):
    """Commands supported by the CLP package's archive-manager."""

    FIND_COMMAND = "find"
    DEL_COMMAND = "del"


class ClpPackageArchiveManagerDelSubcommand(StrEnum):
    """Subcommands supported by the CLP package's archive-manager 'del' command."""

    BY_IDS_COMMAND = "by-ids"
    BY_FILTER_COMMAND = "by-filter"


class ArchiveManagerArgs(CmdArgs):
    """Command argument model for running archive-manager."""

    script_path: Path
    config: Path
    dataset: str | None = None
    command: str
    del_subcommand: str | None = None
    ids: list[str] | None = None
    begin_ts: int | None = None
    end_ts: int | None = None

    def to_cmd(self) -> list[str]:
        """Converts the model attributes to a command list."""
        cmd: list[str] = [
            str(self.script_path),
            "--config",
            str(self.config),
        ]

        if self.dataset:
            cmd.append("--dataset")
            cmd.append(self.dataset)

        cmd.append(self.command)

        if self.del_subcommand:
            cmd.append(self.del_subcommand)
        if self.ids:
            cmd.extend(self.ids)
        if self.begin_ts is not None:
            cmd.append("--begin-ts")
            cmd.append(str(self.begin_ts))
        if self.end_ts is not None:
            cmd.append("--end-ts")
            cmd.append(str(self.end_ts))

        return cmd


def run_archive_manager_find(
    clp_package: ClpPackage,
    dataset: SampleDataset | None = None,
    begin_ts: int | None = None,
    end_ts: int | None = None,
) -> ClpAction:
    """
    Runs an archive-manager 'find' operation.

    :param clp_package:
    :param dataset:
    :param begin_ts:
    :param end_ts:
    :return: The `ClpAction` instance that runs the 'find'.
    """
    args = ArchiveManagerArgs(
        script_path=clp_package.path_config.archive_manager_path,
        config=clp_package.temp_config_file_path,
        command=ClpPackageArchiveManagerCommand.FIND_COMMAND,
        dataset=dataset.dataset_name if dataset is not None else None,
        begin_ts=begin_ts,
        end_ts=end_ts,
    )
    return ClpAction.from_args(args)


def run_archive_manager_del_by_ids(
    clp_package: ClpPackage,
    ids: list[str],
    dataset: SampleDataset | None = None,
) -> ClpAction:
    """
    Runs an archive-manager 'del by-ids' operation against the given list of archive IDs.

    :param clp_package:
    :param ids:
    :param dataset:
    :return: The `ClpAction` instance that runs the 'del by-ids'.
    """
    args = ArchiveManagerArgs(
        script_path=clp_package.path_config.archive_manager_path,
        config=clp_package.temp_config_file_path,
        command=ClpPackageArchiveManagerCommand.DEL_COMMAND,
        del_subcommand=ClpPackageArchiveManagerDelSubcommand.BY_IDS_COMMAND,
        dataset=dataset.dataset_name if dataset is not None else None,
        ids=ids,
    )
    return ClpAction.from_args(args)


def run_archive_manager_del_by_filter(
    clp_package: ClpPackage,
    dataset: SampleDataset | None = None,
    begin_ts: int | None = None,
    end_ts: int | None = None,
) -> ClpAction:
    """
    Runs an archive-manager 'del by-filter' operation over the given time range.

    :param clp_package:
    :param dataset:
    :param begin_ts:
    :param end_ts:
    :return: The `ClpAction` instance that runs the 'del by-filter'.
    """
    args = ArchiveManagerArgs(
        script_path=clp_package.path_config.archive_manager_path,
        config=clp_package.temp_config_file_path,
        command=ClpPackageArchiveManagerCommand.DEL_COMMAND,
        del_subcommand=ClpPackageArchiveManagerDelSubcommand.BY_FILTER_COMMAND,
        dataset=dataset.dataset_name if dataset is not None else None,
        begin_ts=begin_ts,
        end_ts=end_ts,
    )
    return ClpAction.from_args(args)
