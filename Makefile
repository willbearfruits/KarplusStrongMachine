# Digital Kalimba - 7-Button Karplus-Strong Instrument
TARGET = DigitalKalimba

# Sources
CPP_SOURCES = DigitalKalimba.cpp

# Library Locations
LIBDAISY_DIR = $(HOME)/DaisyExamples/libDaisy
DAISYSP_DIR = $(HOME)/DaisyExamples/DaisySP

# Core location
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
