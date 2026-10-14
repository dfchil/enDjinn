CC ?= clang
PYTHON ?= python3
ROOT := ..
TARGET := core_safety_test
TEXTURE_TARGET := texture_shim_test
CPP_API_TARGET := public_api_cpp_test
RESOURCE_TARGET := resource_safety_test
TRANSLUCENT_SORT_TARGET := translucent_sort_test
WEB_FRAME_TARGET := web_frame_test
MODIFIER_VISUAL_BUILD := $(CURDIR)/build/deep-modifiers-pc
MODIFIER_VISUAL_BINARY := $(MODIFIER_VISUAL_BUILD)/enj_deep_modifiers
MODIFIER_BENCH_APP := $(CURDIR)/apps/modifier_benchmark
MODIFIER_BENCH_BUILD := $(CURDIR)/build/modifier-benchmark-pc
MODIFIER_BENCH_BINARY := $(MODIFIER_BENCH_BUILD)/modifier_benchmark
PUBLIC_HEADERS := $(notdir $(wildcard $(ROOT)/include/enDjinn/*.h))
CFLAGS := -std=gnu23 -Wall -Wextra -Werror \
	-I$(ROOT)/backends/pc-endjinn/include -I$(ROOT)/include \
	-DENJ_CBASEPATH=\"/tmp/\"

check: public-headers $(CPP_API_TARGET) $(TARGET) $(RESOURCE_TARGET) \
		$(TEXTURE_TARGET) $(TRANSLUCENT_SORT_TARGET) $(WEB_FRAME_TARGET)
	./$(CPP_API_TARGET)
	./$(TARGET)
	./$(RESOURCE_TARGET)
	./$(TEXTURE_TARGET)
	./$(TRANSLUCENT_SORT_TARGET)
	./$(WEB_FRAME_TARGET)
	$(PYTHON) dctrace_decoder_test.py

public-headers:
	@for header in $(PUBLIC_HEADERS); do \
		$(CC) $(CFLAGS) -DENJ_INJECT_QFONT=1 -fsyntax-only \
			-include enDjinn/$$header -x c /dev/null || exit 1; \
	done

$(TARGET): core_safety_test.c $(ROOT)/code/enj_mode.c $(ROOT)/code/enj_bitmap.c
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(RESOURCE_TARGET): resource_safety_test.c $(ROOT)/code/enj_texture.c
	$(CC) $(CFLAGS) $^ -o $@

$(TEXTURE_TARGET): texture_shim_test.cpp $(ROOT)/backends/host-common/host_pvr.cpp
	$(CXX) -std=c++23 -Wall -Wextra -Werror \
		-I$(ROOT)/backends/pc-endjinn/include $^ -o $@

$(TRANSLUCENT_SORT_TARGET): translucent_sort_test.cpp \
		$(ROOT)/backends/host-common/host_translucent_sort.cpp
	$(CXX) -std=c++23 -Wall -Wextra -Werror \
		-I$(ROOT)/backends/host-common -I$(ROOT)/backends/pc-endjinn/include \
		$^ -o $@

$(WEB_FRAME_TARGET): web_frame_test.cpp \
		$(ROOT)/backends/web-endjinn/web_endjinn_frame.cpp \
		$(ROOT)/backends/host-common/host_pvr.cpp \
		$(ROOT)/backends/host-common/host_translucent_sort.cpp
	$(CXX) -std=c++23 -Wall -Wextra -Werror \
		-I$(ROOT)/backends/host-common \
		-I$(ROOT)/backends/pc-endjinn/include $^ -o $@

modifier-volume-visual:
	$(MAKE) -C $(ROOT)/examples/enj_deep_modifiers ENJ_TARGET=pc-endjinn \
		ENJ_HOST_BUILD_DIR=$(MODIFIER_VISUAL_BUILD) \
		ENJ_HOST_BINDIR=$(MODIFIER_VISUAL_BUILD) \
		ENJ_HOST_TARGET=$(MODIFIER_VISUAL_BINARY) \
		ENJ_HOST_APP_CPPFLAGS=-DENJ_DEEP_MODIFIERS_TEST_CONTROLS
	$(PYTHON) modifier_volume_visual_regression.py \
		--binary $(MODIFIER_VISUAL_BINARY)

modifier-volume-visual-update:
	$(MAKE) -C $(ROOT)/examples/enj_deep_modifiers ENJ_TARGET=pc-endjinn \
		ENJ_HOST_BUILD_DIR=$(MODIFIER_VISUAL_BUILD) \
		ENJ_HOST_BINDIR=$(MODIFIER_VISUAL_BUILD) \
		ENJ_HOST_TARGET=$(MODIFIER_VISUAL_BINARY) \
		ENJ_HOST_APP_CPPFLAGS=-DENJ_DEEP_MODIFIERS_TEST_CONTROLS
	$(PYTHON) modifier_volume_visual_regression.py \
		--binary $(MODIFIER_VISUAL_BINARY) --update-reference

modifier-volume-web:
	$(MAKE) -C $(ROOT)/examples/enj_deep_modifiers ENJ_TARGET=web-endjinn

modifier-volume-web-visual: modifier-volume-web
	node web_modifier_visual_regression.mjs

modifier-volume-benchmark:
	$(MAKE) -C $(MODIFIER_BENCH_APP) \
		-f $(abspath $(ROOT)/base_link.mk) ENJ_TARGET=pc-endjinn \
		ENJ_HOST_BUILD_DIR=$(MODIFIER_BENCH_BUILD) \
		ENJ_HOST_BINDIR=$(MODIFIER_BENCH_BUILD) \
		ENJ_HOST_TARGET=$(MODIFIER_BENCH_BINARY)
	$(PYTHON) modifier_volume_benchmark.py $(MODIFIER_BENCH_BINARY)

$(CPP_API_TARGET): public_api_cpp_test.cpp
	$(CXX) -std=c++23 -Wall -Wextra -Werror \
		-I$(ROOT)/backends/pc-endjinn/include -I$(ROOT)/include \
		-DENJ_CBASEPATH=\"/tmp/\" -DENJ_INJECT_QFONT=1 $< -o $@

clean:
	rm -f $(TARGET) $(TEXTURE_TARGET) $(CPP_API_TARGET) $(RESOURCE_TARGET) \
		$(TRANSLUCENT_SORT_TARGET) $(WEB_FRAME_TARGET)
	rm -rf build

.PHONY: check clean public-headers modifier-volume-visual \
	modifier-volume-visual-update modifier-volume-web \
	modifier-volume-web-visual modifier-volume-benchmark
