all:
	@for dir in */; do \
		$(MAKE) -C $$dir || exit; \
	done
clean:
	@for dir in */; do \
		$(MAKE) -C $$dir clean || exit; \
	done

mrproper:
	@for dir in */; do \
		$(MAKE) -C $$dir mrproper || exit; \
	done

.PHONY: all clean mrproper
