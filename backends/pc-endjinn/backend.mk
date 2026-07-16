PC_ENDJINN_BACKEND_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
ENJ_HOST_BUILD_DIR ?= build/pc-endjinn
PC_ENDJINN_HOST_OS ?= $(shell uname -s)
PKG_CONFIG ?= pkg-config

ifeq ($(PC_ENDJINN_HOST_OS),Darwin)
HOMEBREW_PREFIX ?= /opt/homebrew
QSB ?= $(HOMEBREW_PREFIX)/bin/qsb
SDL2_CONFIG ?= sdl2-config
MOLTENVK_PREFIX ?= $(HOMEBREW_PREFIX)/opt/molten-vk

SDL_CFLAGS ?= $(shell $(SDL2_CONFIG) --cflags)
SDL_LIBS ?= $(shell $(SDL2_CONFIG) --libs)
VULKAN_CFLAGS ?= -I$(MOLTENVK_PREFIX)/libexec/include
VULKAN_LIBS ?= -L$(MOLTENVK_PREFIX)/lib -lMoltenVK \
	-framework Metal -framework IOSurface -framework QuartzCore \
	-framework IOKit -framework Foundation
else ifeq ($(PC_ENDJINN_HOST_OS),Linux)
QSB ?= qsb
SDL_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags sdl2)
SDL_LIBS ?= $(shell $(PKG_CONFIG) --libs sdl2)
VULKAN_CFLAGS ?= $(shell $(PKG_CONFIG) --cflags vulkan)
VULKAN_LIBS ?= $(shell $(PKG_CONFIG) --libs vulkan)
PC_ENDJINN_SYSTEM_LIBS ?= -lm
else
$(error Unsupported pc-enDjinn host '$(PC_ENDJINN_HOST_OS)')
endif

PC_ENDJINN_PLATFORM_SRCS := \
	$(PC_ENDJINN_BACKEND_DIR)kos_abi_compat.cpp \
	$(PC_ENDJINN_BACKEND_DIR)enj_platform_pc_endjinn.cpp \
	$(PC_ENDJINN_BACKEND_DIR)pc_endjinn_pvr.cpp \
	$(PC_ENDJINN_BACKEND_DIR)pc_endjinn_vulkan.cpp \
	$(PC_ENDJINN_BACKEND_DIR)pc_endjinn_input.cpp \
	$(PC_ENDJINN_BACKEND_DIR)pc_endjinn_fs.cpp
PC_ENDJINN_KOS_HEADERS := $(shell find $(PC_ENDJINN_BACKEND_DIR)include \
	-type f -name '*.h')
PC_ENDJINN_BACKEND_HEADERS := $(wildcard $(PC_ENDJINN_BACKEND_DIR)*.h)
PC_ENDJINN_KOS_ABI_CONTRACT := \
	$(PC_ENDJINN_BACKEND_DIR)include/pc_endjinn/kos_abi_contract.generated.h
PC_ENDJINN_CPPFLAGS := -I$(PC_ENDJINN_BACKEND_DIR)include \
	-DENJ_TARGET_PC_ENDJINN=1 $(SDL_CFLAGS) $(VULKAN_CFLAGS)
PC_ENDJINN_LDFLAGS := $(SDL_LIBS) $(VULKAN_LIBS) $(PC_ENDJINN_SYSTEM_LIBS)

pc-endjinn-kos-abi-contract:
	$(PC_ENDJINN_BACKEND_DIR)tools/generate_kos_abi_contract.sh \
		$(PC_ENDJINN_KOS_ABI_CONTRACT) \
		$(ENJ_HOST_BUILD_DIR)/kos-abi

.PHONY: pc-endjinn-kos-abi-contract
ENJ_HOST_EXTRA_DEPS += \
	$(ENJ_HOST_BUILD_DIR)/flat.vert.spv \
	$(ENJ_HOST_BUILD_DIR)/flat.frag.spv \
	$(ENJ_HOST_BUILD_DIR)/flat_punch.frag.spv

$(ENJ_HOST_BUILD_DIR)/%.vert.spv: $(PC_ENDJINN_BACKEND_DIR)shaders/%.vert | $(ENJ_HOST_BUILD_DIR)
	@mkdir -p $(dir $@)
	$(QSB) $< -o $@.qsb
	$(QSB) -x spirv,100 $@.qsb -o $@

$(ENJ_HOST_BUILD_DIR)/%.frag.spv: $(PC_ENDJINN_BACKEND_DIR)shaders/%.frag | $(ENJ_HOST_BUILD_DIR)
	@mkdir -p $(dir $@)
	$(QSB) $< -o $@.qsb
	$(QSB) -x spirv,100 $@.qsb -o $@
