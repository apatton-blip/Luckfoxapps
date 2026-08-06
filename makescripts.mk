.PHONY: all build flash run clean common

SDK_DIR := /home
SCRIPTS_DIR := $(SDK_DIR)/development/scripts
APP ?= main_app

all: build flash run

build flash run clean sssh: common
	@chmod +x $(SCRIPTS_DIR)/$@.sh;\
	APP=$(APP) $(SCRIPTS_DIR)/$@.sh

common:
	@chmod +x $(SCRIPTS_DIR)/$@.sh;