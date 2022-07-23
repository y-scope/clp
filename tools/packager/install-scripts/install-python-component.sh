#!/bin/bash

# Exit on error
set -e

cUsage="Usage: ${BASH_SOURCE[0]} <component-name>"
if [ "$#" -lt 1 ] ; then
    echo "$cUsage"
    exit 1
fi
component_name=$1
python_package_name=${component_name//-/_}

echo "Installing ${component_name}"

cd "${WORKING_DIR}/${component_name}" || exit 1

num_reqs_processed=0
while IFS= read -r -d '' req ; do
  if [ -z "$req" ] ; then
    continue
  fi

  PIP_CACHE_DIR=${CACHE_DIR} pip3 install \
    -c constraints.txt
    --target "${WORKING_DIR}/${ARTIFACT_NAME}/lib/python3/site-packages" \
    "$req" &

  (( ++num_reqs_processed ))
  if (( num_reqs_processed % BUILD_PARALLELISM == 0)) ; then
    # Wait after starting every $BUILD_PARALLELISM jobs
    wait
  fi
done < <(sed 's/#.*//' requirements.txt | tr '\n' '\0')
# Wait for remaining jobs to finish
wait

cp -R "${python_package_name}" "${WORKING_DIR}/${ARTIFACT_NAME}/lib/python3/site-packages"
