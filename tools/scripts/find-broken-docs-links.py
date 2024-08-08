import os
import subprocess
import sys
from pathlib import Path


def main(argv):
    repo_root = _get_repo_root()

    found_violation = False

    # Check for docs.yscope.com links with ".md" suffixes
    if _check_tracked_files(
        r"docs\.yscope\.com/.+\.md",
        repo_root,
        repo_root,
        'docs.yscope.com links cannot have ".md" suffixes.',
    ):
        found_violation = True

    # Check for sphinx :link: attributes that have ".md" suffixes
    if _check_tracked_files(
        r":link:[[:space:]]*.+\.md",
        repo_root,
        repo_root / "docs",
        'sphinx :link: attributes cannot have ".md" suffixes',
    ):
        found_violation = True

    if found_violation:
        return 1

    return 0


def _get_repo_root() -> Path:
    path_str = subprocess.check_output(
        ["git", "rev-parse", "--show-toplevel"], cwd=Path(__file__).parent, text=True
    )
    return Path(path_str.strip())


def _check_tracked_files(
    pattern: str, repo_root: Path, dir_to_search: Path, error_msg: str
) -> bool:
    """
    Check for a pattern in all tracked files in the repo (except this script).
    :param pattern: The pattern to search for.
    :param repo_root: The root of the repository.
    :param dir_to_search: The directory to search in.
    :param error_msg: Error message if the pattern is found.
    :return: Whether the pattern was found in any file.
    """
    found_matches = False

    # NOTE: "-z" ensures the paths won't be quoted (while delimiting them using '\0')
    for path_str in subprocess.check_output(
        [
            "git",
            "ls-files",
            "--cached",
            "--exclude-standard",
            "-z",
            str(dir_to_search.relative_to(repo_root)),
        ],
        cwd=repo_root,
        text=True,
    ).split("\0"):
        path = Path(path_str)

        # Skip directories and this script
        if path == __file__ or (repo_root / path).is_dir():
            continue

        try:
            for match in subprocess.check_output(
                ["grep", "--extended-regexp", "--line-number", "--with-filename", pattern, path],
                cwd=repo_root,
                text=True,
            ).splitlines():
                _parse_and_print_match(match, error_msg)
                found_matches = True
        except subprocess.CalledProcessError:
            pass

    return found_matches


def _parse_and_print_match(match: str, error_msg: str):
    """
    Parses and prints grep matches in a format relevant to the current environment.
    :param match: The match to parse and print.
    :param error_msg: Error message if the pattern is found.
    """
    if os.getenv("GITHUB_ACTIONS") == "true":
        # Print a GitHub Actions error annotation
        file, line, _ = match.split(":", 2)
        print(f"::error file={file},line={line}::{error_msg}")
    else:
        print(error_msg, file=sys.stderr)
        print(match, file=sys.stderr)


if "__main__" == __name__:
    sys.exit(main(sys.argv))
