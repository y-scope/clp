#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail

self_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

path_without_self=""
IFS=':' read -r -a path_parts <<< "${PATH}"
for path_part in "${path_parts[@]}"; do
    [[ "${path_part}" == "${self_dir}" ]] && continue
    if [[ -z "${path_without_self}" ]]; then
        path_without_self="${path_part}"
    else
        path_without_self="${path_without_self}:${path_part}"
    fi
done

real_cmake="$(PATH="${path_without_self}" command -v cmake)"

is_generate=false
for arg in "$@"; do
    case "${arg}" in
        -S|--install|--build|-P|-E)
            [[ "${arg}" == "-S" ]] && is_generate=true
            [[ "${arg}" != "-S" ]] && is_generate=false
            break
            ;;
        -S*)
            is_generate=true
            break
            ;;
    esac
done

if [[ "${is_generate}" == "true" && -n "${CLP_OSXCROSS_CMAKE_ARGS:-}" ]]; then
    # Intentional word splitting: the wrapper receives a shell-style arg string
    # from build-in-osxcross-container.sh so we can inject flags into unmodified
    # taskfile helper calls from the yscope-dev-utils submodule.
    # shellcheck disable=SC2086
    exec "${real_cmake}" "$@" ${CLP_OSXCROSS_CMAKE_ARGS}
fi

exec "${real_cmake}" "$@"
