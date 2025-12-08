all:
	@for dir in */; do \
		$(MAKE) -C $$dir; \
	done
clean:
	@for dir in */; do \
		$(MAKE) -C $$dir clean; \
	done

mrproper:
	@for dir in */; do \
		$(MAKE) -C $$dir mrproper; \
	done

.PHONY: all clean mrproper
