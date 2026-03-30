# Intended use of this file is to symlink to it from the project directory you want to build, and then run make from that directory. This allows you to keep your project files separate from the enDjinn source files, and also allows you to easily update enDjinn without having to copy files around.

ENDDJINNPRIMARY =  $(realpath abspath $(lastword $(MAKEFILE_LIST)))

ENJDIR:= $(dir $(ENDDJINNPRIMARY))
include $(KOS_BASE)/Makefile.rules

# redefine variables in the following in a file ./local.cfg.mk as necessary
include ${ENJDIR}cfg.mk
ifneq (,$(wildcard ./local.cfg.mk))
  include ./local.cfg.mk
endif
ifndef ROMBASEPATH
	ROMBASEPATH:=$(ENJ_ROMDIR)/${ENJ_BASENAME}
endif

ifndef OBJS  # this allows OBJS to be be started in local.cfg.mk
	OBJS :=
endif

OBJS += $(shell find ${ENJDIR}code/ -name '*.c' -not -path "${ENJDIR}.git/*" |sed -e 's,${ENJDIR}\(.*\).c,$(ENJ_BUILDDIR)/enDjinn/\1.o,g')	
OBJS += $(shell find ${ENJ_CODEDIR} -name '*.c' -not -path "./.git/*" |sed -e 's,\.\(.*\).c,$(ENJ_BUILDDIR)\1.o,g')

include ${ENJDIR}texture.mk
include ${ENJDIR}sfx.mk
include ${ENJDIR}fonts.mk

include ${ENJDIR}defines.mk

all: $(ENJ_BINDIR)/$(ENJ_BASENAME).elf
.DEFAULT: all

$(ENJ_BUILDDIR)/enDjinn/%.o: ${ENJDIR}%.c Makefile
	@mkdir -p $(shell dirname $@)
	@echo "Building $@ from $<..."
	@$(ENJ_CC) $(ENJ_INCLUDES) $(ENJ_CFLAGS) $(DEFINES) -c $< -o $@

$(ENJ_BUILDDIR)/%.o: %.c Makefile $(ENJ_TEXTURES) $(ENJ_SNDFXFILES) $(ENJ_FONTFILES)
	@mkdir -p $(shell dirname $@)
	@echo "Building $@ from $<..."
	@$(ENJ_CC) $(ENJ_INCLUDES) $(ENJ_CFLAGS) $(DEFINES) -c $< -o $@

$(ENJ_BINDIR)/${ENJ_BASENAME}.elf: $(OBJS)
	@mkdir -p $(shell dirname $@)
	@echo "Linking $@..."
	@$(ENJ_CC) $(ENJ_INCLUDES) $(KOS_CFLAGS) $(DEFINES) -o $@ $(OBJS) $(ENJ_LDLIBS) 

$(ENJ_BINDIR)/${ENJ_BASENAME}.cdi: $(ENJ_BINDIR)/${ENJ_BASENAME}.elf
	mkdcdisc \
	--name ${ENJ_BASENAME} \
	--elf $< \
	--directory ${ENJ_ROMDIR}/ \
	--release $(shell date  +"%Y%m%d" ) \
	--serial "ENJ01" \
	--output $@ \
	-v 3 \
	--image ${ENJDIR}assets/iplogo.mr \
	--no-padding

$(ENJ_BINDIR)/${ENJ_BASENAME}.bin: $(ENJ_BINDIR)/${ENJ_BASENAME}.elf
	@echo "Creating binary $@ from $<..."
	sh-elf-objcopy -O binary $< $@

clean:
	@rm -rf $(ENJ_BINDIR)/* $(OBJS)

mrproper: clean
	rm -rf $(ENJ_BINDIR)/$(ENJ_BASENAME).elf $(ENJ_BINDIR)/$(ENJ_BASENAME).cdi $(ENJ_BINDIR)/$(ENJ_BASENAME).bin
	rm -rf $(ENJ_TEXTURES) $(ENJ_SNDFXFILES)  $(addsuffix .pal,$(ENJ_TEXTURES)) $(ENJ_FONTFILES)
	rm -f ${FONTMACHINE}
	find . -type d -empty -delete


.PRECIOUS: $(ENJ_TEXTURES) $(ENJ_SNDFXFILES) $(ENJ_FONTFILES)
.PHONY: list help info

include ${ENJDIR}info.mk
