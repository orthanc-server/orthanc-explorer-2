#!/bin/bash

set -o errexit
set -o xtrace


SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

nginx -p $SCRIPT_DIR -c $SCRIPT_DIR/nginx-dev.conf -s stop


