list:
	@LC_ALL=C $(MAKE) -pRrq -f $(firstword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/(^|\n)# Files(\n|$$)/,/(^|\n)# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | grep -E -v -e '^[^[:alnum:]]' -e '^$@$$'

cfg_info:
	@echo "enDjinn build configuration ($(ENJ_TARGET)):"
	@echo "  ENJDIR              = $(ENJDIR)"
	@echo "  ENJ_BASENAME        = $(ENJ_BASENAME)"
	@echo "  ENJ_CODEDIR         = $(ENJ_CODEDIR)"
	@echo "  ENJ_ROMDIR          = $(ENJ_ROMDIR)"
	@echo "  ROMBASEPATH         = $(ROMBASEPATH)"
	@echo "  ENJ_BUILDDIR        = $(ENJ_BUILDDIR)"
	@echo "  ENJ_BINDIR          = $(ENJ_BINDIR)"
	@echo "  ENJ_TEXTURE_SRC_DIR = $(ENJ_TEXTURE_SRC_DIR)"
	@echo "  ENJ_FONTS_SRC_DIR   = $(ENJ_FONTS_SRC_DIR)"
	@echo "  ENJ_SOUNDFX_SRC_DIR = $(ENJ_SOUNDFX_SRC_DIR)"
	@echo "  SOUNDMACHINE        = $(SOUNDMACHINE)"
	@echo "  OPTLEVEL            = $(OPTLEVEL)"
	@echo "  ENJ_INCLUDES        = $(ENJ_INCLUDES)"
	@echo "  ENJ_CFLAGS          = $(ENJ_CFLAGS)"
	@echo "  ENJ_LDLIBS          = $(ENJ_LDLIBS)"
	@echo "  ENJ_LDFLAGS         = $(ENJ_LDFLAGS)"

auto_variables:
	@echo "Generated build inputs:"
	@echo "  ENJ_TEXTURES   = $(ENJ_TEXTURES)"
	@echo "  ENJ_SNDFXFILES = $(ENJ_SNDFXFILES)"
	@echo "  ENJ_FONTFILES  = $(ENJ_FONTFILES)"
	@echo "  DEFINES        = $(DEFINES)"
	@echo "  OBJS           = $(OBJS)"

help:
	@echo "Usage: make [TARGET] [ENJ_TARGET=dreamcast|pc-endjinn|web-endjinn]"
	@echo ""
	@echo "Common targets:"
	@echo "  all             Build the selected backend (default)"
	@echo "  assets          Generate declared project assets"
	@echo "  clean           Remove outputs for the selected backend"
	@echo "  mrproper        Remove generated outputs and assets"
	@echo "  cfg_info        Show resolved configuration"
	@echo "  auto_variables  Show discovered objects and assets"
	@echo "  list            List make targets"
	@echo "  help            Show this help"
	@echo ""
	@echo "Selected backend: $(ENJ_TARGET)"

.PHONY: list cfg_info auto_variables help
