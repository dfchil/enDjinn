# Optional SH4ZAM integration for Dreamcast, native PC, and WebAssembly builds.
# Enable it from an application's local.cfg.mk with ENJ_USE_SH4ZAM := 1.
#
# SH4ZAM owns its source list and platform selection. Native and WebAssembly
# builds therefore consume the library produced by SH4ZAM's CMake project,
# while Dreamcast consumes the library installed by the KOS port.

SH4ZAM_SEARCH_DIRS := \
	$(abspath $(CURDIR)/../sh4zam) \
	$(abspath $(ENJDIR)/../sh4zam) \
	$(abspath $(CURDIR)/third_party/sh4zam) \
	$(abspath $(CURDIR)/vendor/sh4zam)
SH4ZAM_DIR ?= $(firstword $(foreach dir,$(SH4ZAM_SEARCH_DIRS),\
	$(if $(wildcard $(dir)/CMakeLists.txt),$(dir))))

# Keep cleanup and informational targets usable when the optional dependency is
# not checked out. A bare make invocation builds the default `all` target.
SH4ZAM_NONBUILD_GOALS := clean mrproper help info list
SH4ZAM_BUILD_GOALS := $(filter-out $(SH4ZAM_NONBUILD_GOALS),$(MAKECMDGOALS))
ifeq ($(strip $(MAKECMDGOALS)),)
SH4ZAM_BUILD_GOALS := all
endif
ifneq ($(strip $(SH4ZAM_BUILD_GOALS)),)
ifneq ($(ENJ_TARGET),dreamcast)
ifeq ($(wildcard $(SH4ZAM_DIR)/CMakeLists.txt),)
$(error SH4ZAM integration is enabled, but its source tree was not found. Clone sh4zam next to enDjinn or set SH4ZAM_DIR=/absolute/path/to/sh4zam)
endif
endif
endif

ifneq ($(wildcard $(SH4ZAM_DIR)/CMakeLists.txt),)
SH4ZAM_SOURCES := $(shell find $(SH4ZAM_DIR)/source $(SH4ZAM_DIR)/include -type f)
else
SH4ZAM_SOURCES :=
endif

SH4ZAM_CMAKE ?= cmake
SH4ZAM_CMAKE_BUILD_TYPE ?= Release
SH4ZAM_FEATURE_MAKEFILE := $(lastword $(MAKEFILE_LIST))

ifeq ($(ENJ_TARGET),pc-endjinn)
SH4ZAM_HOST_BUILD_DIR ?= build/pc-endjinn/sh4zam
SH4ZAM_HOST_CMAKE_CACHE := $(SH4ZAM_HOST_BUILD_DIR)/CMakeCache.txt
SH4ZAM_HOST_LIB := $(SH4ZAM_HOST_BUILD_DIR)/libsh4zam.a

# Keep the real SH4ZAM headers ahead of the PC backend's minimal compatibility
# headers so an enabled integration can never silently select the stub.
ENJ_HOST_CPPFLAGS := -I$(SH4ZAM_DIR)/include $(ENJ_HOST_CPPFLAGS)
ENJ_HOST_EXTRA_OBJS += $(SH4ZAM_HOST_LIB)

$(SH4ZAM_HOST_CMAKE_CACHE): $(SH4ZAM_DIR)/CMakeLists.txt $(SH4ZAM_FEATURE_MAKEFILE)
	$(SH4ZAM_CMAKE) -S $(SH4ZAM_DIR) -B $(SH4ZAM_HOST_BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(SH4ZAM_CMAKE_BUILD_TYPE) \
		-DSHZ_ENABLE_TESTS=OFF

$(SH4ZAM_HOST_LIB): $(SH4ZAM_HOST_CMAKE_CACHE) $(SH4ZAM_SOURCES)
	$(SH4ZAM_CMAKE) --build $(SH4ZAM_HOST_BUILD_DIR) --target sh4zam
else ifeq ($(ENJ_TARGET),web-endjinn)
ENJ_WEB_BUILD_DIR ?= build/web-endjinn
SH4ZAM_WEB_BUILD_DIR ?= $(ENJ_WEB_BUILD_DIR)/sh4zam-cmake
SH4ZAM_WEB_CMAKE_CACHE := $(SH4ZAM_WEB_BUILD_DIR)/CMakeCache.txt
SH4ZAM_WEB_LIB := $(SH4ZAM_WEB_BUILD_DIR)/libsh4zam.a
SH4ZAM_EMCMAKE ?= emcmake

ENJ_WEB_CPPFLAGS := -I$(SH4ZAM_DIR)/include -DSHZ_TLS_MODEL=SHZ_TLS_IMPLICIT $(ENJ_WEB_CPPFLAGS)
ENJ_WEB_EXTRA_OBJS += $(SH4ZAM_WEB_LIB)

$(SH4ZAM_WEB_CMAKE_CACHE): $(SH4ZAM_DIR)/CMakeLists.txt $(SH4ZAM_FEATURE_MAKEFILE)
	$(SH4ZAM_EMCMAKE) $(SH4ZAM_CMAKE) -S $(SH4ZAM_DIR) -B $(SH4ZAM_WEB_BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(SH4ZAM_CMAKE_BUILD_TYPE) \
		-DCMAKE_C_FLAGS:STRING="$(ENJ_WEB_SIMD_FLAGS)" \
		-DSHZ_ENABLE_TESTS=OFF \
		-DSHZ_TLS_MODEL=IMPLICIT

$(SH4ZAM_WEB_LIB): $(SH4ZAM_WEB_CMAKE_CACHE) $(SH4ZAM_SOURCES)
	$(SH4ZAM_CMAKE) --build $(SH4ZAM_WEB_BUILD_DIR) --target sh4zam
else ifeq ($(ENJ_TARGET),dreamcast)
ENJ_LDLIBS += -lsh4zam
else
$(error SH4ZAM integration does not support ENJ_TARGET '$(ENJ_TARGET)')
endif
