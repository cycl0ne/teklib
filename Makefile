
# TEKlib source directory:
TEKLIB ?= .

# Target platform or standard [posix, winnt]:
PLATFORM ?= posix

# Operating system implementing the platform [linux, fbsd, winnt]:
HOST ?= linux

# Compiler [gcc]:
COMPILER ?= gcc

###############################################################################

help:
	@echo "==============================================================================="
	@echo "Build configuration (pass as variables or override in Makefile):"
	@echo "-------------------------------------------------------------------------------"
	@echo "TEKLIB = ................ $(TEKLIB)"
	@echo "PLATFORM = .............. $(PLATFORM) [available: posix, winnt]"
	@echo "HOST = .................. $(HOST) [available: linux, fbsd, winnt]"
	@echo "COMPILER = .............. $(COMPILER) [available: gcc]"
	@echo "==============================================================================="
	@echo "Standard build targets:"
	@echo "-------------------------------------------------------------------------------"
	@echo "all, $(HOST) .............. default build"
	@echo "release ................. release build"
	@echo "debug ................... debug build"
	@echo "install ................. install"
	@echo "make .................... regenerate makefiles"
	@echo "clean ................... delete executables, modules, object files"
	@echo "==============================================================================="
	@echo "Extra build targets:"
	@echo "-------------------------------------------------------------------------------"
	@echo "docs .................... (re-)generate documentation"
	@echo "distclean ............... delete all temporary files"
	@echo "==============================================================================="

all: $(HOST)
release: $(HOST)_release
debug: $(HOST)_debug
install: $(HOST)_install
make: $(HOST)_make
clean: $(HOST)_clean
sources: $(HOST)_sources
distclean:
	-rm -Rf bin lib
	-find . -type d | grep "build/obj_" | xargs rm -r
	-find . -type f | grep "build/tmk_" | xargs rm

###############################################################################

TMKMF = $(TEKLIB)/bin/$(HOST)/tmkmf

build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER): $(TMKMF)
	$(TEKLIB)/bin/$(HOST)/tmkmf -q -r -c $(PLATFORM)_$(HOST)_$(COMPILER)
build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER)_release: $(TMKMF)
	$(TEKLIB)/bin/$(HOST)/tmkmf -q -r -c $(PLATFORM)_$(HOST)_$(COMPILER)_release
build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER)_debug: $(TMKMF)
	$(TEKLIB)/bin/$(HOST)/tmkmf -q -r -c $(PLATFORM)_$(HOST)_$(COMPILER)_debug
$(HOST): build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER)
	@TEKLIB=$(TEKLIB) $(MAKE) -s -f $? all
$(HOST)_clean: build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER)
	@TEKLIB=$(TEKLIB) $(MAKE) -s -f $? clean
$(HOST)_release: build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER)_release
	@TEKLIB=$(TEKLIB) $(MAKE) -s -f $? all
$(HOST)_install: build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER)
	@TEKLIB=$(TEKLIB) $(MAKE) -s -f $? install
$(HOST)_debug: build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER)_debug
	@TEKLIB=$(TEKLIB) $(MAKE) -s -f $? all
$(HOST)_modules: build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER)
	@TEKLIB=$(TEKLIB) $(MAKE) -s -f $? modules
$(HOST)_libs: build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER)
	@TEKLIB=$(TEKLIB) $(MAKE) -s -f $? libs
$(HOST)_make: $(TMKMF)
	$(TEKLIB)/bin/$(HOST)/tmkmf -q -r -c $(PLATFORM)_$(HOST)_$(COMPILER)
	$(TEKLIB)/bin/$(HOST)/tmkmf -q -r -c $(PLATFORM)_$(HOST)_$(COMPILER)_debug
	$(TEKLIB)/bin/$(HOST)/tmkmf -q -r -c $(PLATFORM)_$(HOST)_$(COMPILER)_release
$(HOST)_sources: build/tmk_$(PLATFORM)_$(HOST)_$(COMPILER)
	@TEKLIB=$(TEKLIB) $(MAKE) -s -f $? sources

###############################################################################

$(TEKLIB)/bin/linux/tmkmf:
	-@mkdir -p $(TEKLIB)/bin/linux
	$(CC) -Wall -I$(TEKLIB) -I$(TEKLIB)/src/hal $(TEKLIB)/build/tmkmf.c -pthread -ldl -lm -o $@

$(TEKLIB)/bin/fbsd/tmkmf:
	-@mkdir -p $(TEKLIB)/bin/fbsd
	$(CC) -Wall -I$(TEKLIB) -I$(TEKLIB)/src/hal $(TEKLIB)/build/tmkmf.c -export-dynamic -pthread -lm -o $@

$(TEKLIB)/bin/winnt/tmkmf:
	-@mkdir -p $(TEKLIB)/bin/winnt
# 	$(CC) -Wall -I$(TEKLIB) -I$(TEKLIB)/src/hal $(TEKLIB)/build/tmkmf.c -mno-cygwin -lm -lwinmm -o $@
	$(CC) -Wall -I$(TEKLIB) -I$(TEKLIB)/src/hal $(TEKLIB)/build/tmkmf.c -pthread -ldl -lm -o $@

# Playstation2 is cross-built on Linux:
$(TEKLIB)/bin/ps2/tmkmf:
	-@mkdir -p $(TEKLIB)/bin/ps2
	$(CC) -I$(TEKLIB) -I$(TEKLIB)/src/hal $(TEKLIB)/build/tmkmf.c -pthread -ldl -lm -o $@

###############################################################################

kdiff:
	-(a=$$(mktemp -du) && hg clone $$PWD $$a && kdiff3 $$a $$PWD; rm -rf $$a)

docs:
	etc/gendoc.lua . -e -n TEKlib reference manual > doc/manual.html
