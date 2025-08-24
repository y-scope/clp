#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

# Get the installed go-task version string
task_version_line=$(task --version 2>/dev/null | head -n1)

# Check it is go-task, not Taskwarrior
if [[ "${task_version_line}" != Task\ version* ]]; then
  echo "Error: The 'task' binary resolves to a different program (not go-task)."
  echo "Got: '${task_version_line}'"
  exit 1
fi

# Parse the version string
grep_pattern="(?:v)?\K[0-9]+\.[0-9]+\.[0-9]+"
task_version=$(echo "${task_version_line}" | grep --only-matching --perl-regexp "${grep_pattern}")
task_major_version=$(echo "${task_version}" | cut -d. -f1)
task_minor_version=$(echo "${task_version}" | cut -d. -f2)

# Check version constraints
if [ "$task_major_version" -ne "3" ] || \
   [ "$task_minor_version" -lt "40" ] || \
   [ "$task_minor_version" -ge "43" ]; then
  echo "Error: Task version ${task_version} is currently unsupported (must be 3.40 <= ver < 3.43)."
  exit 1
fi
