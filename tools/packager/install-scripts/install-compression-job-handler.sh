#!/bin/bash
echo "Installing clp-compression-job-handler"
cd ${WORKING_DIR}/clp-compression-job-handler
xargs --max-args=1 --max-procs=16 pip install --target ${WORKING_DIR}/${ARTIFACT_NAME}/lib/python3/site-packages < requirements.txt
cp -R compression_job_handler ${WORKING_DIR}/${ARTIFACT_NAME}/lib/python3/site-packages
