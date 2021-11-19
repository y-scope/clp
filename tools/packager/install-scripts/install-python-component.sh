#!/bin/bash

cUsage="Usage: ${BASH_SOURCE[0]} <component-name>"
if [ "$#" -lt 1 ] ; then
    echo $cUsage
    exit 1
fi
component_name=$1
python_package_name=${component_name//-/_}

echo "Installing ${component_name}"

cd ${WORKING_DIR}/${component_name}

PIP_CACHE_DIR=${CACHE_DIR} xargs --max-args=1 --max-procs=16 \
  pip install --target ${WORKING_DIR}/${ARTIFACT_NAME}/lib/python3/site-packages < requirements.txt

cp -R ${python_package_name} ${WORKING_DIR}/${ARTIFACT_NAME}/lib/python3/site-packages
