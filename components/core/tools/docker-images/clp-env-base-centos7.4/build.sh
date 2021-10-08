#!/bin/bash

script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
component_root=${script_dir}/../../../

docker build -t clp-env-base:centos7.4 ${component_root} --file ${script_dir}/Dockerfile
