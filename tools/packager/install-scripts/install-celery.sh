#!/bin/bash

echo "Installing celery"

pip3 install celery==5.1.2

bin_dir=${WORKING_DIR}/${ARTIFACT_NAME}/bin/
mkdir -p ${bin_dir}
cp /usr/local/bin/celery ${bin_dir}
