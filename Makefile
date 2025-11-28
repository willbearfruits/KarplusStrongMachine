# Digital Kalimba - 7-Button Karplus-Strong Instrument
TARGET = DigitalKalimba

# Use SRAM bootloader for 480KB program space (3.7x more than internal flash)
# Options: BOOT_QSPI (8MB), BOOT_SRAM (480KB), or omit for internal 128KB
# APP_TYPE = BOOT_SRAM  # Disabled - SRAM linker has .bss overflow issue with ReverbSc

# Enable LGPL library for ReverbSc
USE_DAISYSP_LGPL = 1

# Sources
CPP_SOURCES = DigitalKalimba.cpp

# Library Locations
LIBDAISY_DIR = $(HOME)/DaisyExamples/libDaisy
DAISYSP_DIR = $(HOME)/DaisyExamples/DaisySP

# ============================================
# MEMORY OPTIMIZATION FLAGS (DISABLED - BROKE AUDIO)
# ============================================
# Optimize for size instead of speed (-Os vs -O2)
# OPT = -Os

# Link-Time Optimization (5-20% code size reduction) - BREAKS AUDIO!
# CFLAGS += -flto
# CPPFLAGS += -flto
# LDFLAGS += -flto

# Fast math for audio DSP (safe, saves Flash)
# CPPFLAGS += -ffast-math -fsingle-precision-constant

# Additional size optimizations
# CPPFLAGS += -fno-inline-small-functions -fno-unroll-loops

# Core location
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
