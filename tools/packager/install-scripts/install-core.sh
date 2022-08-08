#!/bin/bash

echo "Installing CLP core"

build_dir=/tmp/core-build

mkdir ${build_dir}
cd ${build_dir}

exes="clg clp clo"

cmake ${WORKING_DIR}/core
make -j${BUILD_PARALLELISM} ${exes}

bin_dir=${WORKING_DIR}/${ARTIFACT_NAME}/bin/
mkdir -p ${bin_dir}
cp ${exes} ${bin_dir}
