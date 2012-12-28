# ydfs Makefile
# Copyright (C) 2012 yue cheng <yue.cheng.ted@gmail.com>
# This file is released under the GNU GPL v3 license, see the COPYING file
#
# Dependencies are stored in the Makefile.dep file. To rebuild this file Just use 'make dep', but this is only needed by developers.

VPATH= ../common:../tracker

LIB = pthread

FINAL_CFLAGS+= -l$(LIB) -I../common -I../tracker
FINAL_LDFLAGS+= -$(LIB)

YDFS_CC=$(QUIET_CC)$(CC) $(FINAL_CFLAGS)
YDFS_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)

YDFS_STORAGE_NAME= ydfs_storage
YDFS_STORAGE_OBJ= ydfs_storage.o storage_service.o storage_net.o aio.o trunk_file.o \
			storage_sync.o storage_nio.o storage_recovery.o\
		../common/debug.o ../common/ae.o ../common/sockopt.o ../common/log.o \
		../common/base.o ../common/mem_pool.o ../common/share_net.o
YDFS_STORAGE_CONFIG=storage.config
YDFS_STORAGE_1_CONFIG=storage_1.config
YDFS_STORAGE_2_CONFIG=storage_2.config

all: $(YDFS_STORAGE_NAME)

.PHONY: all

# Deps (use make dep to generate this)
include Makefile.dep

# ydfs_storage
$(YDFS_STORAGE_NAME): $(YDFS_STORAGE_OBJ)
	$(YDFS_LD) -o $@ $^

dep:
	$(YDFS_CC) -MM *.c > Makefile.d
	sed 's,h$$,h\n\t$$(YDFS_CC) -c $$\<,g' <Makefile.d> Makefile.dep;
	rm -f Makefile.d

.PHONY: dep

clean:
	rm -f $(YDFS_STORAGE_NAME) *.o

.PHONY: clean

clean_other:
	rm -rf *~ 0* log binlog 1 2

double_test_storage:
	-mkdir 1
	cp $(YDFS_STORAGE_NAME) 1
	cp $(YDFS_STORAGE_1_CONFIG) 1/$(YDFS_STORAGE_CONFIG)
	-mkdir 2
	cp $(YDFS_STORAGE_NAME) 2
	cp $(YDFS_STORAGE_2_CONFIG) 2/$(YDFS_STORAGE_CONFIG)
.PHONY: clean_other
