# enDgine Makefile for Dreamcast/KallistiOS
# Requires KallistiOS environment to be set up

# Target binary name
TARGET = enDgine.elf

# Source files
OBJS = main.o

# KallistiOS base directory (should be set in environment)
KOS_BASE ?= /opt/toolchains/dc/kos

# Include KallistiOS build rules
all: rm-elf $(TARGET)

include $(KOS_BASE)/Makefile.rules

# Clean target
clean: rm-elf
	-rm -f $(OBJS)

# Remove ELF target
rm-elf:
	-rm -f $(TARGET)

# Compile the target
$(TARGET): $(OBJS)
	kos-cc -o $(TARGET) $(OBJS)

# Run target (for emulators)
run: $(TARGET)
	lxdream $(TARGET)

# Additional compiler flags
KOS_CFLAGS += -std=c99

.PHONY: all clean rm-elf run
