include config.mk
CC = diet -Os gcc

OBJ = sinit.o
BIN = sinit
CFLAGS = -Os -flto -s -fno-asynchronous-unwind-tables -fno-unwind-tables
LDFLAGS = -flto -Os -static -Wl,--gc-sections -Wl,--strip-all -Wl,-z,norelro -fno-asynchronous-unwind-tables -Wl,--build-id=none

all: $(BIN) poweroff reboot

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)
	strip -R .comment -R .jcr -R .eh_data -R .note -R .eh_frame $@

$(OBJ): config.h

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(BIN) $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(MANPREFIX)/man8
	sed "s/VERSION/$(VERSION)/g" < $(BIN).8 > $(DESTDIR)$(MANPREFIX)/man8/$(BIN).8

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)
	rm -f $(DESTDIR)$(MANPREFIX)/man8/$(BIN).8

dist: clean
	mkdir -p sinit-$(VERSION)
	cp LICENSE Makefile README config.def.h config.mk sinit.8 sinit.c sinit-$(VERSION)
	tar -cf sinit-$(VERSION).tar sinit-$(VERSION)
	gzip sinit-$(VERSION).tar
	rm -rf sinit-$(VERSION)

clean:
	rm -f $(BIN) $(OBJ) sinit-$(VERSION).tar.gz

.SUFFIXES: .def.h

.def.h.h:
	cp $< $@

.PHONY:
	all install uninstall dist clean
