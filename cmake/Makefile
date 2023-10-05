mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(patsubst %/,%,$(dir $(mkfile_path)))
debug_out_dir := $(mkfile_dir)/../Debug
release_out_dir := $(mkfile_dir)/../Release

ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
devnul := NUL
which := where
else
devnul := /dev/null
which := which
endif

ifeq ($(GENERATOR),)
ifneq ($(shell ${which} ninja 2>${devnul}),)
GENERATOR := "Ninja"
else
GENERATOR := "Unix Makefiles"
endif
endif

all: cmake-release cmake-debug

$(release_out_dir) $(debug_out_dir):
	@echo ">>> create '$(subst ${CURDIR}/,,$(abspath $@))' as it does not exist already"
	@mkdir $@

cmake-release: $(release_out_dir)
	@echo ">>> enter '$(subst ${CURDIR}/,,$(abspath $<))' and run 'cmake -DCMAKE_BUILD_TYPE=Release -G $(GENERATOR) ..'"
	@cd $(release_out_dir) && cmake -DCMAKE_BUILD_TYPE=Release -G $(GENERATOR) ..

cmake-debug: $(debug_out_dir)
	@echo ">>> enter '$(subst ${CURDIR}/,,$(abspath $<))' and run 'cmake -DCMAKE_BUILD_TYPE=Debug -G $(GENERATOR) ..'"
	@cd $(debug_out_dir) && cmake -DCMAKE_BUILD_TYPE=Debug -G $(GENERATOR) ..

.PHONY: cmake-release cmake-debug
