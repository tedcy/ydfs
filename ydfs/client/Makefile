# ydfs Makefile
# Copyright (C) 2012 yue cheng <yue.cheng.ted@gmail.com>
# This file is released under the GNU GPL v3 license, see the COPYING file
#
# Dependencies are stored in the Makefile.dep file. To rebuild this file Just use 'make dep', but this is only needed by developers.

VPATH= ../common:../tracker:../storage

LIB = pthread

FINAL_CFLAGS+= -l$(LIB) -I../common -I../storage -I../tracker
FINAL_LDFLAGS+= -$(LIB)

YDFS_CC=$(QUIET_CC)$(CC) $(FINAL_CFLAGS)
YDFS_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)

YDFS_CLIENT_NAME= ydfs_client
YDFS_CLIENT_OBJ= ydfs_client.o client_api.o thread_pool.o \
		../common/sockopt.o ../common/log.o ../common/ae.o ../common/base.o \
		../common/debug.o

all: $(YDFS_CLIENT_NAME)

.PHONY: all

# Deps (use make dep to generate this)
include Makefile.dep

# ydfs_client
$(YDFS_CLIENT_NAME): $(YDFS_CLIENT_OBJ)
	$(YDFS_LD) -o $@ $^

dep:
	$(YDFS_CC) -MM *.c > Makefile.d
	sed 's,h$$,h\n\t$$(YDFS_CC) -c $$\<,g' <Makefile.d> Makefile.dep;
	rm -f Makefile.d

.PHONY: dep

clean:
	rm -f $(YDFS_CLIENT_NAME) *.o

.PHONY: clean

clean_other:
	rm -f *~ 00* uploadlog log

.PHONY: clean_other
