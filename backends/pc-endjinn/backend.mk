PC_ENDJINN_BACKEND_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
ENJ_HOST_BUILD_DIR ?= build/pc-endjinn
QSB ?= /opt/homebrew/bin/qsb

HOMEBREW_PREFIX ?= /opt/homebrew
MOLTENVK_PREFIX ?= $(HOMEBREW_PREFIX)/Cellar/molten-vk/1.4.1

SDL_CFLAGS ?= -I$(HOMEBREW_PREFIX)/include/SDL2 -D_THREAD_SAFE
SDL_LIBS ?= -L$(HOMEBREW_PREFIX)/lib -lSDL2
VULKAN_CFLAGS ?= -I$(MOLTENVK_PREFIX)/libexec/include
VULKAN_LIBS ?= -L$(HOMEBREW_PREFIX)/lib -lMoltenVK \
	-framework Metal -framework IOSurface -framework QuartzCore \
	-framework IOKit -framework Foundation

PC_ENDJINN_PLATFORM_SRCS := \
	$(PC_ENDJINN_BACKEND_DIR)kos_abi_compat.cpp \
	$(PC_ENDJINN_BACKEND_DIR)enj_platform_pc_endjinn.cpp \
	$(PC_ENDJINN_BACKEND_DIR)pc_endjinn_input.cpp
PC_ENDJINN_KOS_HEADER := $(PC_ENDJINN_BACKEND_DIR)include/kos.h
PC_ENDJINN_KOS_ABI_CONTRACT := \
	$(PC_ENDJINN_BACKEND_DIR)include/pc_endjinn/kos_abi_contract.generated.h
PC_ENDJINN_CPPFLAGS := -I$(PC_ENDJINN_BACKEND_DIR)include \
	-DENJ_TARGET_PC_ENDJINN=1 $(SDL_CFLAGS) $(VULKAN_CFLAGS)
PC_ENDJINN_LDFLAGS := $(SDL_LIBS) $(VULKAN_LIBS)

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
