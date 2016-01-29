dircontents = $(filter-out makefile,$(shell ls -B -F))

dirs = $(filter %/,$(dircontents))
files = $(filter-out $(dirs),$(dircontents))
target_files = $(addprefix $(target),$(files))

.PHONY: all
all: directory $(target_files)
	@for dir in $(dirs); do \
		cd $${dir} && $(MAKE) target=../$(target)$${dir}; \
		cd ..; \
	done;

$(target)%: %
	cp -fu $< $@

directory:
	@mkdir -p $(target)
