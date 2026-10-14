WEB_ENDJINN_BACKEND_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
EMCC ?= emcc
EMXX ?= em++

ENJ_WEB_BUILD_DIR ?= build/web-endjinn
ENJ_WEB_BINDIR ?= $(ENJ_WEB_BUILD_DIR)
ENJ_WEB_TARGET ?= $(ENJ_WEB_BINDIR)/$(ENJ_BASENAME).html
WEB_ENDJINN_POST_JS := $(WEB_ENDJINN_BACKEND_DIR)web_endjinn.js

ENJ_WEB_CPPFLAGS += \
	-I$(WEB_ENDJINN_BACKEND_DIR) \
	-I$(ENJDIR)backends/host-common \
	-I$(ENJDIR)backends/pc-endjinn/include \
	-DENJ_TARGET_WEB_ENDJINN=1 \
	-sUSE_SDL=2

ENJ_WEB_LDFLAGS += \
	-sUSE_SDL=2 \
	-sMIN_WEBGL_VERSION=2 \
	-sMAX_WEBGL_VERSION=2 \
	-sALLOW_MEMORY_GROWTH=1 \
	-sFORCE_FILESYSTEM=1 \
	-sEXIT_RUNTIME=1 \
	-lidbfs.js \
	--post-js $(WEB_ENDJINN_POST_JS)

ENJ_WEB_EXTRA_DEPS += $(WEB_ENDJINN_POST_JS)

WEB_ENDJINN_PLATFORM_SRCS := \
	$(WEB_ENDJINN_BACKEND_DIR)enj_platform_web_endjinn.cpp \
	$(WEB_ENDJINN_BACKEND_DIR)web_endjinn_frame.cpp \
	$(WEB_ENDJINN_BACKEND_DIR)web_endjinn_input.cpp \
	$(WEB_ENDJINN_BACKEND_DIR)web_endjinn_webgl.cpp
ENJ_HOST_COMMON_DIR := $(ENJDIR)backends/host-common
ENJ_HOST_COMMON_SRCS := \
	$(ENJ_HOST_COMMON_DIR)/host_kos_abi.cpp \
	$(ENJ_HOST_COMMON_DIR)/host_input.cpp \
	$(ENJ_HOST_COMMON_DIR)/host_audio.cpp \
	$(ENJ_HOST_COMMON_DIR)/host_fs.cpp \
	$(ENJ_HOST_COMMON_DIR)/host_stubs.cpp \
	$(ENJ_HOST_COMMON_DIR)/host_pvr.cpp \
	$(ENJ_HOST_COMMON_DIR)/host_translucent_sort.cpp
ENJ_HOST_COMMON_HEADERS := $(wildcard $(ENJ_HOST_COMMON_DIR)/*.h)
