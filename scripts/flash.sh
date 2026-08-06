#!/bin/bash

source /home/development/scripts/common.sh

EXEC_PATH=${APP_BUILD_DIR}/${APP}

scp ${EXEC_PATH} root@${DEFAULT_IP}:${LUCKFOX_HOME_DIR}/${APP}