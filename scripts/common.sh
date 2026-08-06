#!/bin/bash

# ANSI Escape Colors

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\x1b[33m'
CLEAR='\033[0m'

# Board

DEFAULT_IP=172.32.0.93

# Board Paths

LUCKFOX_HOME_DIR=/root

# Local Paths

SDK_DIR=/home

DEVELOPMENT_DIR=${SDK_DIR}/development
DEV_BUILD_DIR="${DEVELOPMENT_DIR}/build"

APPS_DIR="${DEVELOPMENT_DIR}/applications"
APP_DIR="${APPS_DIR}/${APP}"
APP_BUILD_DIR="${APP_DIR}/build"