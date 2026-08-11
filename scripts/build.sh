#!/bin/bash

source /home/development/scripts/common.sh

if_cc_add(){
    if [[ ! -f "${APP_BUILD_DIR}/compile_commands.json" ]]; then
        echo -e "${YELLOW}compile_commands.json could not be generated for this build.${CLEAR}"
        exit 1
    fi

    if [[ ! -d "${DEV_BUILD_DIR}" ]]; then
        mkdir -p "${DEV_BUILD_DIR}"
    fi

    cp "${APP_BUILD_DIR}/compile_commands.json" "${DEV_BUILD_DIR}/compile_commands.json"
    echo -e "${GREEN}compile_commands.json updated successfully.${CLEAR}"
}

on_failure(){
    echo -e "${RED}Build Finished with Errors.${CLEAR}"
}

trap 'on_failure' ERR
trap 'if_cc_add' EXIT

set -e

cd "${APP_DIR}"

echo Starting Build...

${CMAKE_BUILD_SCRIPT}

cmake --build build

echo -e "${GREEN}Build Finished Sucessfully.${CLEAR}"