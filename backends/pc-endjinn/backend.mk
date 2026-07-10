PC_ENDJINN_BACKEND_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

HOMEBREW_PREFIX ?= /opt/homebrew
MOLTENVK_PREFIX ?= $(HOMEBREW_PREFIX)/Cellar/molten-vk/1.4.1

SDL_CFLAGS ?= -I$(HOMEBREW_PREFIX)/include/SDL2 -D_THREAD_SAFE
SDL_LIBS ?= -L$(HOMEBREW_PREFIX)/lib -lSDL2
VULKAN_CFLAGS ?= -I$(MOLTENVK_PREFIX)/libexec/include
VULKAN_LIBS ?= -L$(HOMEBREW_PREFIX)/lib -lMoltenVK \
	-framework Metal -framework IOSurface -framework QuartzCore \
	-framework IOKit -framework Foundation

PC_ENDJINN_PLATFORM_SRC := $(PC_ENDJINN_BACKEND_DIR)enj_platform_pc_endjinn.cpp
PC_ENDJINN_CPPFLAGS := -DENJ_TARGET_PC_ENDJINN=1 $(SDL_CFLAGS) $(VULKAN_CFLAGS)
PC_ENDJINN_LDFLAGS := $(SDL_LIBS) $(VULKAN_LIBS)
