#!/bin/bash
echo "Installing clp-py-utils"
cd ${WORKING_DIR}/clp-py-utils
xargs --max-args=1 --max-procs=16 pip install --target ${WORKING_DIR}/${ARTIFACT_NAME}/lib/python3/site-packages < requirements.txt
cp -R clp_py_utils ${WORKING_DIR}/${ARTIFACT_NAME}/lib/python3/site-packages