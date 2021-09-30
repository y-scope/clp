#!/bin/bash
echo "Installing job-orchestration"
cd ${WORKING_DIR}/job-orchestration
xargs --max-args=1 --max-procs=16 pip install --target ${WORKING_DIR}/${ARTIFACT_NAME}/lib/python3/site-packages < requirements.txt
cp -R job_orchestration ${WORKING_DIR}/${ARTIFACT_NAME}/lib/python3/site-packages