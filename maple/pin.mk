# Rules for the PIN binary instrumentation tool.

ifndef PIN_HOME
  $(error Please define PIN_HOME environment.)
endif

PIN_KIT = $(PIN_HOME)
PIN_ROOT := $(PIN_HOME)
CONFIG_ROOT = $(PIN_ROOT)/source/tools/Config
KIT = 1

TARGET_COMPILER ?= gnu
ifdef OS
  ifeq (${OS}, Windows_NT)
    TARGET_COMPILER = ms
  endif
endif

ifeq ($(TARGET_COMPILER), gnu)
  #include $(PIN_HOME)/source/tools/makefile.gnu.config
  # Possibly uncomment when using newer version of pin
  include $(PIN_HOME)/source/tools/Config/unix.vars
  include $(PIN_HOME)/source/tools/Config/makefile.unix.config
  #include $(PIN_HOME)/source/tools/Config/makefile.default.rules
  CXXFLAGS ?= -Wall -Wno-unknown-pragmas $(DBG) $(OPT)
  #-Werror 
  PIN = $(PIN_HOME)/pin
endif

ifeq ($(TARGET_COMPILER), ms)
  include $(PIN_HOME)/source/tools/makefile.ms.config
  DBG ?=
  PIN = $(PIN_HOME)/pin.bat
endif

