# Intended use of this file is to symlink to it from the project directory you
# want to build, and then run make from that directory. This keeps project files
# separate from enDjinn source files and allows enDjinn to be updated in place.

ENJ_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
ENJ_REAL_MAKEFILE := $(realpath $(ENJ_MAKEFILE))
ENJDIR := $(dir $(ENJ_REAL_MAKEFILE))
ENJ_BASEDIR := $(ENJDIR)
ENJ_TARGET ?= dreamcast

# Redefine variables in ./local.cfg.mk as necessary.
include ${ENJDIR}cfg.mk
ifneq (,$(wildcard ./local.cfg.mk))
  include ./local.cfg.mk
endif
.DEFAULT_GOAL := all

ifeq ($(ENJ_TARGET),pc-endjinn)
include ${ENJDIR}backends/pc-endjinn/backend.mk

CC ?= clang
CXX ?= clang++
ENJ_CC := $(CC)
ENJ_CXX := $(CXX)

ENJ_HOST_BUILD_DIR ?= build/pc-endjinn
ENJ_HOST_BINDIR ?= build/pc-endjinn
ENJ_HOST_TARGET ?= $(ENJ_HOST_BINDIR)/$(ENJ_BASENAME)

ENJ_HOST_CFLAGS ?= -std=gnu23 -O2 -g -Wall -Wextra \
	-Wno-unused-function -Wno-unused-variable
ENJ_HOST_CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -pedantic \
	-Wno-c99-extensions -Wno-missing-field-initializers -Wno-unused-function

ENJ_HOST_CPPFLAGS += \
	-I$(shell pwd)/include \
	-I${ENJDIR}include \
	$(PC_ENDJINN_CPPFLAGS)

ifndef ENJ_CBASEPATH
	ENJ_CBASEPATH := $(abspath $(ENJ_ROMDIR)/$(ENJ_BASENAME))/
endif
include ${ENJDIR}defines.mk

ENJ_HOST_CORE_SRCS ?= \
	${ENJDIR}code/enj_bitmap.c \
	${ENJDIR}code/enj_ctrl.c \
	${ENJDIR}code/enj_draw.c \
	${ENJDIR}code/enj_mode.c \
	${ENJDIR}code/enj_render.c \
	${ENJDIR}code/enj_rumble.c \
	${ENJDIR}code/enj_sound.c \
	${ENJDIR}code/enj_state.c

ENJ_HOST_APP_SRCS ?= $(shell find $(ENJ_CODEDIR) -name '*.c' -not -path "./.git/*")
ENJ_HOST_CORE_OBJS := $(patsubst ${ENJDIR}%.c,$(ENJ_HOST_BUILD_DIR)/enDjinn/%.o,$(ENJ_HOST_CORE_SRCS))
ENJ_HOST_APP_OBJS := $(patsubst %.c,$(ENJ_HOST_BUILD_DIR)/%.o,$(ENJ_HOST_APP_SRCS))
ENJ_HOST_BACKEND_OBJ := $(ENJ_HOST_BUILD_DIR)/enDjinn/backends/pc-endjinn/enj_platform_pc_endjinn.o
ENJ_HOST_OBJS := $(ENJ_HOST_CORE_OBJS) $(ENJ_HOST_APP_OBJS) $(ENJ_HOST_BACKEND_OBJ) $(ENJ_HOST_EXTRA_OBJS)

all: pc-endjinn
.DEFAULT: all

pc-endjinn: $(ENJ_HOST_TARGET)
pc-endjinn-objects: $(ENJ_HOST_OBJS) $(ENJ_HOST_EXTRA_DEPS)

$(ENJ_HOST_BUILD_DIR):
	mkdir -p $@

$(ENJ_HOST_BUILD_DIR)/enDjinn/%.o: ${ENJDIR}%.c $(ENJ_MAKEFILE) | $(ENJ_HOST_BUILD_DIR)
	@mkdir -p $(dir $@)
	$(ENJ_CC) $(ENJ_HOST_CFLAGS) $(ENJ_HOST_CPPFLAGS) $(DEFINES) -c $< -o $@

$(ENJ_HOST_BUILD_DIR)/%.o: %.c $(ENJ_MAKEFILE) | $(ENJ_HOST_BUILD_DIR)
	@mkdir -p $(dir $@)
	$(ENJ_CC) $(ENJ_HOST_CFLAGS) $(ENJ_HOST_CPPFLAGS) $(ENJ_HOST_APP_CPPFLAGS) $(DEFINES) -c $< -o $@

$(ENJ_HOST_BACKEND_OBJ): $(PC_ENDJINN_PLATFORM_SRC) ${ENJDIR}include/enDjinn/enj_platform.h | $(ENJ_HOST_BUILD_DIR)
	@mkdir -p $(dir $@)
	$(ENJ_CXX) $(ENJ_HOST_CXXFLAGS) $(ENJ_HOST_CPPFLAGS) $(DEFINES) -c $< -o $@

$(ENJ_HOST_TARGET): $(ENJ_HOST_OBJS) $(ENJ_HOST_EXTRA_DEPS)
	@mkdir -p $(dir $@)
	$(ENJ_CXX) $(ENJ_HOST_CXXFLAGS) $(ENJ_HOST_CPPFLAGS) $(DEFINES) \
		$(ENJ_HOST_OBJS) -o $@ $(PC_ENDJINN_LDFLAGS)

clean:
	rm -rf $(ENJ_HOST_BUILD_DIR)

mrproper: clean
	rm -rf $(ENJ_HOST_BINDIR)
	find . -type d -empty -delete

.PHONY: all pc-endjinn pc-endjinn-objects clean mrproper list help info
include ${ENJDIR}info.mk

else ifeq ($(ENJ_TARGET),dreamcast)
include $(KOS_BASE)/Makefile.rules

ifndef ROMBASEPATH
	ROMBASEPATH:=$(ENJ_ROMDIR)/${ENJ_BASENAME}
endif

ifndef OBJS  # this allows OBJS to be be started in local.cfg.mk
	OBJS :=
endif

OBJS += $(shell find ${ENJDIR}code/ -name '*.c' -not -path "${ENJDIR}.git/*" |sed -e 's,${ENJDIR}\(.*\).c,$(ENJ_BUILDDIR)/enDjinn/\1.o,g')	
OBJS += $(shell find ${ENJ_CODEDIR} -name '*.c' -not -path "./.git/*" |sed -e 's,\.\(.*\).c,$(ENJ_BUILDDIR)\1.o,g')

include ${ENJDIR}texture.mk
include ${ENJDIR}sfx.mk
include ${ENJDIR}fonts.mk

include ${ENJDIR}defines.mk

all: $(ENJ_BINDIR)/$(ENJ_BASENAME).elf
.DEFAULT: all

$(ENJ_BUILDDIR)/enDjinn/%.o: ${ENJDIR}%.c $(ENJ_MAKEFILE)
	@mkdir -p $(shell dirname $@)
	@echo "Building $@ from $<..."
	@$(ENJ_CC) $(ENJ_INCLUDES) $(ENJ_CFLAGS) $(DEFINES) -c $< -o $@

$(ENJ_BUILDDIR)/%.o: %.c $(ENJ_MAKEFILE) $(ENJ_TEXTURES) $(ENJ_SNDFXFILES) $(ENJ_FONTFILES)
	@mkdir -p $(shell dirname $@)
	@echo "Building $@ from $<..."
	$(ENJ_CC) $(ENJ_INCLUDES) $(ENJ_CFLAGS) $(DEFINES) -c $< -o $@

$(ENJ_BINDIR)/${ENJ_BASENAME}.elf: $(OBJS)
	@mkdir -p $(shell dirname $@)
	@echo "Linking $@..."
	$(ENJ_CC) $(ENJ_INCLUDES) $(KOS_CFLAGS) $(DEFINES) -o $@ $(OBJS) $(ENJ_LDLIBS) 

$(ENJ_BINDIR)/${ENJ_BASENAME}.cdi: $(ENJ_BINDIR)/${ENJ_BASENAME}.elf
	mkdcdisc \
	--name ${ENJ_BASENAME} \
	--elf $< \
	--directory ${ENJ_ROMDIR}/ \
	--release $(shell date  +"%Y%m%d" ) \
	--serial "ENJ01" \
	--output $@ \
	-v 3 \
	--image ${ENJDIR}assets/iplogo.mr \
	--no-padding

$(ENJ_BINDIR)/${ENJ_BASENAME}.bin: $(ENJ_BINDIR)/${ENJ_BASENAME}.elf
	@echo "Creating binary $@ from $<..."
	sh-elf-objcopy -O binary $< $@

clean:
	@rm -rf $(ENJ_BINDIR)/* $(OBJS)

mrproper: clean
	rm -rf $(ENJ_BINDIR)/$(ENJ_BASENAME).elf $(ENJ_BINDIR)/$(ENJ_BASENAME).cdi $(ENJ_BINDIR)/$(ENJ_BASENAME).bin
	rm -rf $(ENJ_TEXTURES) $(ENJ_SNDFXFILES)  $(addsuffix .pal,$(ENJ_TEXTURES)) $(ENJ_FONTFILES)
	rm -f ${FONTMACHINE}
	find . -type d -empty -delete

.PRECIOUS: $(ENJ_TEXTURES) $(ENJ_SNDFXFILES) $(ENJ_FONTFILES)
.PHONY: all clean mrproper list help info
include ${ENJDIR}info.mk
else
$(error Unknown ENJ_TARGET '$(ENJ_TARGET)'; use ENJ_TARGET=dreamcast or ENJ_TARGET=pc-endjinn)
endif
