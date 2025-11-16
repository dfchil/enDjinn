# Makefile for enDgine - Sega Dreamcast Gameplay Loop Driver
# Requires KallistiOS (KOS) environment

# KallistiOS base path (set your KOS_BASE environment variable)
ifndef KOS_BASE
$(error KOS_BASE is not set. Please set it to your KallistiOS installation path)
endif

# Include KOS build system
include $(KOS_BASE)/Mk/kos.build.mk

# Project settings
TARGET = endgine-demo.elf
LIBRARY = libendgine.a

# Source files
LIB_OBJS = endgine.o
EXAMPLE_OBJS = example.o

# Compiler flags
KOS_CFLAGS += -std=c99 -Wall -Wextra

# Build targets
all: $(LIBRARY) $(TARGET)

# Build the library
$(LIBRARY): $(LIB_OBJS)
	$(KOS_AR) rcs $@ $^
	@echo "Built library: $(LIBRARY)"

# Build the example
$(TARGET): $(EXAMPLE_OBJS) $(LIBRARY)
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $@ $(EXAMPLE_OBJS) -L. -lengine $(KOS_LIBS)
	@echo "Built example: $(TARGET)"

# Clean build artifacts
clean:
	-rm -f $(LIB_OBJS) $(EXAMPLE_OBJS) $(LIBRARY) $(TARGET) romdisk.*
	@echo "Cleaned build artifacts"

# Install library
install: $(LIBRARY)
	@if [ -z "$(DESTDIR)" ]; then \
		echo "DESTDIR not set. Skipping install."; \
	else \
		mkdir -p $(DESTDIR)/lib $(DESTDIR)/include; \
		cp $(LIBRARY) $(DESTDIR)/lib/; \
		cp endgine.h $(DESTDIR)/include/; \
		echo "Installed to $(DESTDIR)"; \
	fi

# Documentation target
.PHONY: all clean install

# Pattern rules
%.o: %.c
	$(KOS_CC) $(KOS_CFLAGS) -c $< -o $@
