# compile logic
CROSS_COMPILE	= riscv64-unknown-linux-gnu
CC				= $(CROSS_COMPILE)-gcc
OBJDUMP			= $(CROSS_COMPILE)-objdump

DIR_PWD			= $(shell pwd)
DIR_BUILD		= $(DIR_PWD)/build


.PHONY: all

all: build lib


build:
	mkdir -p $(DIR_BUILD)


lib:



