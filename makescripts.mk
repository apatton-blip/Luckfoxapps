.PHONY: all build flash run clean common

SDK_DIR := /home
SCRIPTS_DIR := $(SDK_DIR)/development/scripts
APP ?= main_app

CMAKE_BUILD_SCRIPT ?= "cmake -B build"

all: build flash run

flash run clean sssh: common
	@chmod +x $(SCRIPTS_DIR)/$@.sh;\
	APP=$(APP) $(SCRIPTS_DIR)/$@.sh

build: common
	@chmod +x $(SCRIPTS_DIR)/$@.sh;\
	APP=$(APP) CMAKE_BUILD_SCRIPT=$(CMAKE_BUILD_SCRIPT) $(SCRIPTS_DIR)/$@.sh

common:
	@chmod +x $(SCRIPTS_DIR)/$@.sh;