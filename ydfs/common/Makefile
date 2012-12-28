# ydfs Makefile
# Copyright (C) 2012 yue cheng <yue.cheng.ted@gmail.com>
# This file is released under the GNU GPL v3 license, see the COPYING file
#
# Dependencies are stored in the Makefile.dep file. To rebuild this file Just use 'make dep', but this is only needed by developers.

FINAL_CFLAGS+= -lpthread

YDFS_CC=$(QUIET_CC)$(CC) $(FINAL_CFLAGS)
YDFS_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)

YDFS_STORAGE_OBJ= debug.o ae.o sockopt.o log.o base.o mem_pool.o share_net.o

all: $(YDFS_STORAGE_OBJ)

.PHONY: all

# Deps (use make dep to generate this)
-include Makefile.dep

dep:
	$(YDFS_CC) -MM *.c > Makefile.d
	sed 's,h$$,h\n\t$$(YDFS_CC) -c $$\<,g' <Makefile.d> Makefile.dep;
	rm -f Makefile.d

.PHONY: dep

clean:
	rm -f *.o

.PHONY: clean

clean_other:
	rm -f *~

.PHONY: clean_other
