ROMBASESFXDIR:=$(notdir $(ENJ_SOUNDFX_SRC_DIR))

SOUNDMACHINE ?= dcaconv
# A supplied path participates in dependency tracking. A command found on PATH
# does not name a project-local make target.
ENJ_SOUNDMACHINE_DEP := $(if $(findstring /,$(SOUNDMACHINE)),$(SOUNDMACHINE),)

ENJ_SNDFXFILES:=
ifneq (,$(wildcard $(ENJ_SOUNDFX_SRC_DIR)))
	ENJ_SNDFXFILES += $(shell find $(ENJ_SOUNDFX_SRC_DIR) -name '*.wav' -or -name '*.mp3'| sed -e 's,'$(ENJ_SOUNDFX_SRC_DIR)',$(ROMBASEPATH)/$(ROMBASESFXDIR),g' | sed -E 's,\.(wav|mp3),\.dca,g')
endif

$(ROMBASEPATH)/$(ROMBASESFXDIR)/ADPCM/%.dca: $(ENJ_SOUNDFX_SRC_DIR)/ADPCM/%.* $(ENJ_SOUNDMACHINE_DEP)
	@mkdir -p $(shell dirname $@)
	$(SOUNDMACHINE)  --format ADPCM -i $< -o $@

$(ROMBASEPATH)/$(ROMBASESFXDIR)/PCM/16/%.dca: $(ENJ_SOUNDFX_SRC_DIR)/PCM/16/%.* $(ENJ_SOUNDMACHINE_DEP)
	@mkdir -p $(shell dirname $@)
	$(SOUNDMACHINE)  --format PCM16 -i $< -o $@

$(ROMBASEPATH)/$(ROMBASESFXDIR)/PCM/8/%.dca: $(ENJ_SOUNDFX_SRC_DIR)/PCM/8/%.* $(ENJ_SOUNDMACHINE_DEP)
	@mkdir -p $(shell dirname $@)
	$(SOUNDMACHINE)  --format PCM8 -i $< -o $@
