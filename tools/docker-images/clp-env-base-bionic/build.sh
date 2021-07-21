#!/bin/bash

project_root="../../.."

scripts=()
scripts+=(libarchive_3.5.1.sh)
scripts+=(lz4_1.8.2.sh)
scripts+=(spdlog_1.8.2.sh)
scripts+=(zstandard_1.4.9.sh)

# Copy package installation scripts into working directory so docker can access them
for script in ${scripts[@]} ; do
  cp ${project_root}/tools/scripts/lib_install/${script} .
done

docker build -t clp-env-base:bionic .

# Delete package installation scripts
rm ${scripts[@]}
