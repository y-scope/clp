#!/usr/bin/env bash

# Exit on any error, use of undefined variables, or failure within a pipeline
set -euo pipefail

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

failures=0

run_check() {
    local script_name="$1"
    local label="$2"
    local script_output

    if script_output=$("${script_dir}/${script_name}" 2>&1); then
        echo "[OK] ${label}"
    else
        echo "[FAIL] ${label}"
        echo "${script_output}"
        failures=$((failures + 1))
    fi
}

# Run all checks
# TODO: https://github.com/y-scope/clp/issues/795
run_check check-cmake-version.sh "CMake version"

# TODO: https://github.com/y-scope/clp/issues/872
run_check check-go-task-version.sh "Go Task version"

run_check check-uv-version.sh "uv version"

# Print summary
echo
if [ "${failures}" -gt 0 ]; then
    echo "${failures} check(s) failed."
    exit 1
else
    echo "All checks passed."
fi
