#!/bin/bash

source /home/development/scripts/common.sh

OUT_DIR=${APP_DIR}/out

scp -r ${OUT_DIR} root@${DEFAULT_IP}:${LUCKFOX_HOME_DIR}/${APP}