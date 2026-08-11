#!/bin/bash

APP=luckfox_pico_rtsp_yolov5
source ../../scripts/common.sh

scp -r ${APP_DIR}/model root@${DEFAULT_IP}:${LUCKFOX_HOME_DIR}