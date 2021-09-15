#!/bin/bash

project_root="../../.."

scripts=()
scripts+=(libarchive.sh)
scripts+=(lz4.sh)
scripts+=(spdlog.sh)
scripts+=(zstandard.sh)

# Copy package installation scripts into working directory so docker can access them
for script in ${scripts[@]} ; do
  cp ${project_root}/tools/scripts/lib_install/${script} .
done

docker build -t clp-env-base:focal .

# Delete package installation scripts
rm ${scripts[@]}
