PREFIX=$(shell echo `pwd`)
PREFIX_DEP=$(PREFIX)/build/imgflo-dependencies/install
VERSION=$(shell echo `git describe --tags`)
CFLAGS += -Wall -Wextra
CFLAGS += -Ofast -g -rdynamic
CFLAGS += -fPIC
CFLAGS += -fomit-frame-pointer
CFLAGS += -funroll-loops -fforce-addr -ftracer -fpeel-loops -fmerge-all-constants
CFLAGS += -Wno-unused-parameter  -Wno-sign-compare
CFLAGS +=  -I. -D_XOPEN_SOURCE=500
CFLAGS += -std=c99

LIBS=gegl-0.3 libsoup-2.4
SYSTEM_LIBS=gio-unix-2.0 json-glib-1.0 libpng OpenEXR
DEPS=$(shell $(PREFIX)/env.sh pkg-config --define-variable=prefix=$(PREFIX_DEP) --libs --cflags $(LIBS))
DEPS+=$(shell $(PREFIX)/env.sh pkg-config --libs --cflags $(SYSTEM_LIBS))
TRAVIS_DEPENDENCIES=$(shell echo `cat .vendor_urls | sed -e "s/heroku/travis-linux/" | tr -d '\n'`)
TARGET=$(shell uname -n)

VIDEO_URL="ftp://vps.jonnor.com/imgflo/printed-electret-microphone.mp4"

all: release

iconographer: env iconographer.c Makefile *.c
	$(PREFIX)/env.sh gcc -o ./build/bin/iconographer *.c -I. $(CFLAGS) $(DEPS)

install: travis-deps iconographer

env:
	mkdir -p $(PREFIX)/build|| true
	mkdir -p $(PREFIX)/build/bin || true
	sed -e 's|dir|$(PREFIX_DEP)|' env.sh.in > $(PREFIX)/env.sh
	chmod +x $(PREFIX)/env.sh

travis-deps:
	wget -O $(PREFIX)/imgflo-dependencies.tgz $(TRAVIS_DEPENDENCIES)
	mkdir -p $(PREFIX)/build/imgflo-dependencies
	tar -xzf $(PREFIX)/imgflo-dependencies.tgz -C $(PREFIX)/build/imgflo-dependencies

clean:
	rm -rf $(PREFIX)/env.sh $(PREFIX)/imgflo-dependencies.tgz

check:
	wget -O $(PREFIX)/video.mp4 $(VIDEO_URL)
	$(PREFIX)/env.sh $(PREFIX)/build/bin/iconographer -p $(PREFIX)/video.mp4 $(PREFIX)/frame.png

release: install check clean
	cd $(PREFIX) && tar -czf iconographer-$(VERSION)-$(TARGET).tgz build
