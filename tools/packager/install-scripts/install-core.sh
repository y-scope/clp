#!/bin/bash
echo "Installing CLP core"
cd ${WORKING_DIR}/core
mkdir build
cd build
cmake ../
make -j${BUILD_PARALLELISM}
mkdir -p ${WORKING_DIR}/${ARTIFACT_NAME}/bin/
cp clp clg ${WORKING_DIR}/${ARTIFACT_NAME}/bin/
