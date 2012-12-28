# ydfs Makefile
# Copyright (C) 2012 yue cheng <yue.cheng.ted@gmail.com>
# This file is released under the GNU GPL v3 license, see the COPYING file
#
# Dependencies are stored in the Makefile.dep file. To rebuild this file Just use 'make dep', but this is only needed by developers.
#
# Top level makefile

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
#OPTIMIZATION?=-O2

# Default settings
STD= -std=c99 -pedantic
#STD= -std=gnu99
WARN= -Wall
OPT= $(OPTIMIZATION)

# Override default settings if possible
#-include .make-settings

#-D_GNU_SOURCE -D_BSD_SOURCE
ifeq ($(uname_S),SunOS)
  FINAL_CFLAGS= $(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS)
  FINAL_LDFLAGS= $(LDFLAGS) -g -ggdb
  DEBUG= -g -ggdb
else
  FINAL_CFLAGS= $(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS)
  FINAL_LDFLAGS= $(LDFLAGS) -g -rdynamic -ggdb
  DEBUG= -g -rdynamic -ggdb
endif

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
BINCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"


ifndef V
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_INSTALL = @printf '    %b %b\n' $(LINKCOLOR)INSTALL$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
endif

export

#real shit
default: all

.DEFAULT:
	@echo ""
	cd common && $(MAKE) $@
	@echo ""
	cd storage && $(MAKE) $@
	@echo ""
	cd tracker && $(MAKE) $@
	@echo ""
	cd client && $(MAKE) $@
	@echo ""

install:
	@echo ""
	cd common && $(MAKE) $@
	@echo ""
	cd storage && $(MAKE) $@
	@echo ""
	cd tracker && $(MAKE) $@
	@echo ""
	cd client && $(MAKE) $@
	@echo ""

.PHONY: install

clean_other:
	rm -f *~
	@echo ""
	cd common && $(MAKE) $@
	@echo ""
	cd storage && $(MAKE) $@
	@echo ""
	cd tracker && $(MAKE) $@
	@echo ""
	cd client && $(MAKE) $@	
	@echo ""

double_test_storage:
	@echo ""
	cd storage && $(MAKE) $@
	@echo ""
.PHONY: clean_other
