CC ?= clang
ROOT := ..
TARGET := core_safety_test
TEXTURE_TARGET := texture_shim_test
CFLAGS := -std=gnu23 -Wall -Wextra -Werror \
	-I$(ROOT)/backends/pc-endjinn/include -I$(ROOT)/include \
	-DENJ_CBASEPATH=\"/tmp/\"

check: $(TARGET) $(TEXTURE_TARGET)
	./$(TARGET)
	./$(TEXTURE_TARGET)

$(TARGET): core_safety_test.c $(ROOT)/code/enj_mode.c $(ROOT)/code/enj_bitmap.c
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(TEXTURE_TARGET): texture_shim_test.cpp $(ROOT)/backends/pc-endjinn/pc_endjinn_pvr.cpp
	$(CXX) -std=c++17 -Wall -Wextra -Werror \
		-I$(ROOT)/backends/pc-endjinn/include $^ -o $@

clean:
	rm -f $(TARGET) $(TEXTURE_TARGET)

.PHONY: check clean
