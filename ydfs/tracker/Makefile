# ydfs Makefile
# Copyright (C) 2012 yue cheng <yue.cheng.ted@gmail.com>
# This file is released under the GNU GPL v3 license, see the COPYING file
#
# Dependencies are stored in the Makefile.dep file. To rebuild this file Just use 'make dep', but this is only needed by developers.

VPATH= ../common

LIB = pthread

FINAL_CFLAGS+= -l$(LIB) -I../common
FINAL_LDFLAGS+= -$(LIB)

YDFS_CC=$(QUIET_CC)$(CC) $(FINAL_CFLAGS)
YDFS_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)

YDFS_TRACKER_NAME= ydfs_tracker
YDFS_TRACKER_OBJ= ydfs_tracker.o tracker_service.o tracker_net.o storage_pool.o tracker_nio.o\
		../common/debug.o ../common/ae.o ../common/sockopt.o ../common/log.o \
		 ../common/base.o ../common/mem_pool.o ../common/share_net.o

all: $(YDFS_TRACKER_NAME)

.PHONY: all

# Deps (use make dep to generate this)
include Makefile.dep

# ydfs_tracker
$(YDFS_TRACKER_NAME): $(YDFS_TRACKER_OBJ)
	$(YDFS_LD) -o $@ $^

dep:
	$(YDFS_CC) -MM *.c > Makefile.d
	sed 's,h$$,h\n\t$$(YDFS_CC) -c $$\<,g' <Makefile.d> Makefile.dep;
	rm -f Makefile.d

.PHONY: dep

clean:
	rm -f $(YDFS_TRACKER_NAME) *.o

.PHONY: clean

clean_other:
	rm -f *~ log

.PHONY: clean_other
