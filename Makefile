PREFIX=$(shell echo `pwd`)
VERSION=$(shell echo `git describe --tags`)
CFLAGS += -Wall -Wextra
CFLAGS += -Ofast -g -rdynamic
CFLAGS += -fPIC
CFLAGS += -fomit-frame-pointer
CFLAGS += -funroll-loops -fforce-addr -ftracer -fpeel-loops -fmerge-all-constants
CFLAGS += -Wno-unused-parameter  -Wno-sign-compare
CFLAGS +=  -I. -D_XOPEN_SOURCE=500
CFLAGS += -std=c99

LIBS=gegl-0.3 libsoup-2.4 gio-unix-2.0 json-glib-1.0 libpng
DEPS=$(shell $(PREFIX)/env.sh pkg-config --define-variable=prefix=$(PREFIX)/imgflo-dependencies/install/ --libs --cflags $(LIBS))
TRAVIS_DEPENDENCIES=$(shell echo `cat .vendor_urls | sed -e "s/heroku/travis-linux/" | tr -d '\n'`)

all: install

iconographer: iconographer.c Makefile *.c
	$(PREFIX)/env.sh gcc -o iconographer *.c -I. $(CFLAGS) $(DEPS)

clean:
	rm iconographer

install: travis-deps env iconographer
	cp iconographer /usr/local/bin

env:
	chmod +x $(PREFIX)/env.sh
	@echo "$$GEGL_PATH"

travis-deps:
	wget -O imgflo-dependencies.tgz $(TRAVIS_DEPENDENCIES)
	mkdir -p $(PREFIX)/imgflo-dependencies
	tar -xvzf imgflo-dependencies.tgz -C $(PREFIX)/imgflo-dependencies

release: install
	cd $(PREFIX) && tar -czf ../iconographer-$(VERSION).tgz ./
