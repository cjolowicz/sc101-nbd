package = sc101-nbd
version = 0.05

SRCS = ut.c psan.c util.c
OBJS = $(SRCS:.c=.o)
HDRS = psan_wireformat.h psan.h util.h nbd.h

DEFINES = -D_GNU_SOURCE

ifneq ($(shell ls -1 /usr/include/linux/nbd.h 2>/dev/null),)
DEFINES += -DUSE_NBD $(if $(shell grep NBD_CMD_READ /usr/include/linux/nbd.h 2>/dev/null),,-DMISSING_COMMANDS)
endif

OPTIM = -g
OPTIM += -O2

CPPFLAGS = $(INCLUDES) $(DEFINES)
CFLAGS = -Wall -pedantic -std=c99 $(OPTIM)

INSTALL = install -D

DESTDIR =
sysconfdir = /etc
sbindir = /sbin
man8dir = /usr/share/man/man8

all: ut

ut: $(OBJS)
	$(CC) -o ut $(OBJS)

include .depend

.depend: Makefile $(SRCS) $(HDRS)
	$(CC) -MM $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) $(SRCS) >.depend

install: ut
	$(INSTALL) -m 0755 ut.init $(DESTDIR)$(sysconfdir)/init.d/ut
	$(INSTALL) -m 0600 /dev/null $(DESTDIR)$(sysconfdir)/uttab
	$(INSTALL) -m 0755 ut $(DESTDIR)$(sbindir)/ut
	$(INSTALL) -m 0644 ut.8 $(DESTDIR)$(man8dir)/ut.8
	gzip --best --force $(DESTDIR)$(man8dir)/*.8

clean:
	rm -f $(OBJS) ut

realclean: clean
	rm -f .depend
	rm -rf dist.tmp deb.tmp

distclean: realclean
	rm -f $(package)_*.tar.gz
	rm -f $(package)_*.changes
	rm -f $(package)_*.dsc
	rm -f $(package)_*.deb

dist: $(package)_$(version).tar.gz

$(package)_$(version).tar.gz: .depend
	! git status -s | grep '^[^?]' # check for changes
	rm -rf dist.tmp
	mkdir -p dist.tmp/$(package)-$(version)
	git ls-files | cpio -dump dist.tmp/$(package)-$(version)
	perl -pi -e 's/\@pkg_version\@/$(version)/g'			\
	    dist.tmp/$(package)-$(version)/sc101-nbd.spec

	GZIP=--best tar cvzCf dist.tmp $(package)_$(version).tar.gz	\
	    $(package)-$(version)

	rm -rf dist.tmp

rpm: dist
	rpmbuild -ta $(package)_$(version).tar.gz

deb: dist
	-rm -rf deb.tmp
	mkdir deb.tmp
	tar xvzfC $(package)_$(version).tar.gz deb.tmp
	cd deb.tmp/$(package)-$(version) &&				\
	    dpkg-buildpackage -rfakeroot -us -uc

	# re-write checksums to use original tar.gz
	perl -mDigest::MD5 -pi.bak					\
	    -e 'BEGIN {'						\
	    -e '    open F, "$(package)_$(version).tar.gz" or die;'	\
	    -e '    $$sz = (stat F)[7];'				\
	    -e '    $$md5 = Digest::MD5->new->addfile(*F)->hexdigest;'	\
	    -e '}'							\
	    -e 'if (/^Files:/..0 and /.tar.gz$$/) {'			\
	    -e '    s/^ ([\da-f]+) (\d+)/ $$md5 $$sz/;'			\
	    -e '}' deb.tmp/*.changes deb.tmp/*.dsc
	cd deb.tmp && mv *.changes *.dsc *.deb ..
	rm -rf deb.tmp

.PHONY: all install clean distclean realclean dist rpm deb
