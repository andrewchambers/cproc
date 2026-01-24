.POSIX:

PREFIX=/usr/local
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man
BACKENDS=amd64 arm64

objdir=.
-include config.mk

.PHONY: all
all: $(objdir)/cproc $(objdir)/cproc-amd64 $(objdir)/cproc-arm64

DRIVER_SRC=\
	driver.c\
	util.c
DRIVER_OBJ=$(DRIVER_SRC:%.c=$(objdir)/%.o)

config.h:
	./configure

$(objdir)/cproc: $(DRIVER_OBJ)
	$(CC) $(LDFLAGS) -o $@ $(DRIVER_OBJ)

COMMON_SRC=\
	attr.c\
	decl.c\
	eval.c\
	expr.c\
	init.c\
	main.c\
	map.c\
	pp.c\
	scan.c\
	scope.c\
	stmt.c\
	targ.c\
	token.c\
	tree.c\
	type.c\
	utf.c\
	util.c
COMMON_OBJ=$(COMMON_SRC:%.c=$(objdir)/%.o)
BACKEND_OBJ=$(BACKENDS:%=$(objdir)/%.o)

$(objdir)/cproc-%: $(COMMON_OBJ) $(objdir)/%.o
	$(CC) $(LDFLAGS) -o $@ $(COMMON_OBJ) $(objdir)/$*.o

$(objdir)/attr.o    : attr.c    util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ attr.c
$(objdir)/decl.o    : decl.c    util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ decl.c
$(objdir)/driver.o  : driver.c  util.h config.h   $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ driver.c
$(objdir)/eval.o    : eval.c    util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ eval.c
$(objdir)/expr.o    : expr.c    util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ expr.c
$(objdir)/init.o    : init.c    util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ init.c
$(objdir)/main.o    : main.c    util.h cc.h arg.h $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ main.c
$(objdir)/map.o     : map.c     util.h            $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ map.c
$(objdir)/pp.o      : pp.c      util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ pp.c
$(objdir)/amd64.o  : amd64.c  util.h cc.h $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ amd64.c
$(objdir)/arm64.o  : arm64.c  util.h cc.h $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ arm64.c
$(objdir)/scan.o    : scan.c    util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ scan.c
$(objdir)/scope.o   : scope.c   util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ scope.c
$(objdir)/stmt.o    : stmt.c    util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ stmt.c
$(objdir)/targ.o    : targ.c    util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ targ.c
$(objdir)/token.o   : token.c   util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ token.c
$(objdir)/tree.o    : tree.c    util.h            $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ tree.c
$(objdir)/type.o    : type.c    util.h cc.h       $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ type.c
$(objdir)/utf.o     : utf.c     utf.h             $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ utf.c
$(objdir)/util.o    : util.c    util.h            $(stagedeps) ; $(CC) $(CFLAGS) -c -o $@ util.c

# Make sure stage2 and stage3 binaries are stripped by adding -s to
# LDFLAGS. Otherwise they will contain paths to object files, which
# differ between stages.

.PHONY: stage2
stage2: all
	@mkdir -p $@
	$(MAKE) objdir=$@ stagedeps='cproc cproc-amd64 cproc-arm64' CC=$(objdir)/cproc LDFLAGS='$(LDFLAGS) -s'

.PHONY: stage3
stage3: stage2
	@mkdir -p $@
	$(MAKE) objdir=$@ stagedeps='stage2/cproc stage2/cproc-amd64 stage2/cproc-arm64' CC=$(objdir)/stage2/cproc LDFLAGS='$(LDFLAGS) -s'

.PHONY: bootstrap
bootstrap: stage2 stage3
	cmp stage2/cproc stage3/cproc
	cmp stage2/cproc-amd64 stage3/cproc-amd64
	cmp stage2/cproc-arm64 stage3/cproc-arm64

.PHONY: check
check: all
	@CCPROC=./cproc ./runtests

.PHONY: install
install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp $(objdir)/cproc $(objdir)/cproc-amd64 $(objdir)/cproc-arm64 $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp cproc.1 $(DESTDIR)$(MANDIR)/man1

.PHONY: clean
clean:
	rm -rf cproc $(DRIVER_OBJ) cproc-amd64 cproc-arm64 $(COMMON_OBJ) $(BACKEND_OBJ) stage2 stage3
