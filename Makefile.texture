
ROMBASETEXTUREDIR:=$(notdir $(ENJ_TEXTURE_SRC_DIR))

ENJ_TEXTURES:=
ifneq (,$(wildcard $(ENJ_TEXTURE_SRC_DIR)))
	ENJ_TEXTURES += $(shell find $(ENJ_TEXTURE_SRC_DIR)/ -name '*.png' | sed -e 's,'$(ENJ_TEXTURE_SRC_DIR)',$(ROMBASEPATH)/$(ROMBASETEXTUREDIR),g' -e 's,\.png,\.dt,g')
endif


ifdef ENJ_ADD_LOGO_TEXTURE
# special case: add the enDjinn logo texture
ENJ_TEXTURES += $(ROMBASEPATH)/$(ROMBASETEXTUREDIR)/pal8/enDjinn512.dt
ENJ_TEXTURES += $(ROMBASEPATH)/$(ROMBASETEXTUREDIR)/pal8/enDjinn256.dt
$(ROMBASEPATH)/$(ROMBASETEXTUREDIR)/pal8/%.dt: ${ENJDIR}/assets/texture/pal8/%.png
	@mkdir -p $(shell dirname $@)
	pvrtex -f PAL8BPP -c --max-color 256 -i $< -o $@
endif


# notice the different settings for different texture formats based on directory names
$(ROMBASEPATH)/$(ROMBASETEXTUREDIR)/pal4/%.dt: $(ENJ_TEXTURE_SRC_DIR)/pal4/%.png
	@mkdir -p $(shell dirname $@)
	pvrtex -f PAL4BPP -c --max-color 16 -i $< -o $@

$(ROMBASEPATH)/$(ROMBASETEXTUREDIR)/pal8/%.dt: $(ENJ_TEXTURE_SRC_DIR)/pal8/%.png
	@mkdir -p $(shell dirname $@)
	pvrtex -f PAL8BPP -c --max-color 256 -i $< -o $@

$(ROMBASEPATH)/$(ROMBASETEXTUREDIR)/rgb565_vq_tw/%.dt: $(ENJ_TEXTURE_SRC_DIR)/rgb565_vq_tw/%.png 
	@mkdir -p $(shell dirname $@)
	pvrtex -f RGB565 -c -i $< -o $@ -p $@.png

$(ROMBASEPATH)/$(ROMBASETEXTUREDIR)/argb1555_vq_tw/%.dt: $(ENJ_TEXTURE_SRC_DIR)/argb1555_vq_tw/%.png
	@mkdir -p $(shell dirname $@)
	pvrtex -f ARGB1555 -c -i $< -o $@
