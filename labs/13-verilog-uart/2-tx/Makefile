ROOT=$(CS240LX_2023_PATH)/fpga
BOARD ?= upduino
BOARDROOT=$(ROOT)/$(BOARD)

SRC=$(wildcard *.sv)
TOP=uart_top
SIM=sim.cc
SIM_TOP=uart_top
SIM_SRC=$(SRC)
VFLAGS += -DCLK_MHZ=48 --public

include $(BOARDROOT)/rules.mk
include $(ROOT)/verilator.mk

screen:
	screen /dev/ttyUSB0 19200
.PHONY: screen

clean:
	$(BOARDCLEAN)
	$(SIMCLEAN)
