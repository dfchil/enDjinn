CC ?= clang
PYTHON ?= python3
ROOT := ..
TARGET := core_safety_test
TEXTURE_TARGET := texture_shim_test
CPP_API_TARGET := public_api_cpp_test
RESOURCE_TARGET := resource_safety_test
TRANSLUCENT_SORT_TARGET := translucent_sort_test
PUBLIC_HEADERS := $(notdir $(wildcard $(ROOT)/include/enDjinn/*.h))
CFLAGS := -std=gnu23 -Wall -Wextra -Werror \
	-I$(ROOT)/backends/pc-endjinn/include -I$(ROOT)/include \
	-DENJ_CBASEPATH=\"/tmp/\"

check: public-headers $(CPP_API_TARGET) $(TARGET) $(RESOURCE_TARGET) $(TEXTURE_TARGET) $(TRANSLUCENT_SORT_TARGET)
	./$(CPP_API_TARGET)
	./$(TARGET)
	./$(RESOURCE_TARGET)
	./$(TEXTURE_TARGET)
	./$(TRANSLUCENT_SORT_TARGET)

public-headers:
	@for header in $(PUBLIC_HEADERS); do \
		$(CC) $(CFLAGS) -DENJ_INJECT_QFONT=1 -fsyntax-only \
			-include enDjinn/$$header -x c /dev/null || exit 1; \
	done

$(TARGET): core_safety_test.c $(ROOT)/code/enj_mode.c $(ROOT)/code/enj_bitmap.c
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(RESOURCE_TARGET): resource_safety_test.c $(ROOT)/code/enj_texture.c
	$(CC) $(CFLAGS) $^ -o $@

$(TEXTURE_TARGET): texture_shim_test.cpp $(ROOT)/backends/pc-endjinn/pc_endjinn_pvr.cpp
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I$(ROOT)/backends/pc-endjinn/include $^ -o $@

$(TRANSLUCENT_SORT_TARGET): translucent_sort_test.cpp \
		$(ROOT)/backends/pc-endjinn/pc_endjinn_translucent_sort.cpp
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I$(ROOT)/backends/pc-endjinn -I$(ROOT)/backends/pc-endjinn/include \
		$^ -o $@

modifier-volume-visual:
	$(MAKE) -C $(ROOT)/examples/enj_modifier_volume_zclip ENJ_TARGET=pc-endjinn
	$(PYTHON) modifier_volume_visual_regression.py

modifier-volume-visual-update:
	$(MAKE) -C $(ROOT)/examples/enj_modifier_volume_zclip ENJ_TARGET=pc-endjinn
	$(PYTHON) modifier_volume_visual_regression.py --update-reference

modifier-volume-web:
	$(MAKE) -C $(ROOT)/examples/enj_modifier_volume_zclip ENJ_TARGET=web-endjinn

$(CPP_API_TARGET): public_api_cpp_test.cpp
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I$(ROOT)/backends/pc-endjinn/include -I$(ROOT)/include \
		-DENJ_CBASEPATH=\"/tmp/\" -DENJ_INJECT_QFONT=1 $< -o $@

clean:
	rm -f $(TARGET) $(TEXTURE_TARGET) $(CPP_API_TARGET) $(RESOURCE_TARGET) \
		$(TRANSLUCENT_SORT_TARGET)

.PHONY: check clean public-headers modifier-volume-visual \
	modifier-volume-visual-update modifier-volume-web
