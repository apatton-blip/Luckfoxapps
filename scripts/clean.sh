#!/bin/bash

source /home/development/scripts/common.sh

if [[ -d "${APP_BUILD_DIR}" ]]; then
    rm -rf "${APP_BUILD_DIR}"
fi

if [[ -f "${DEV_BUILD_DIR}/compile_commands.json" ]]; then
    rm -f "${DEV_BUILD_DIR}/compile_commands.json"
fi