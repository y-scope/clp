#!/usr/bin/env bash

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

"$script_dir"/install-prebuilt-packages.sh
"$script_dir"/install-packages-from-source.sh
