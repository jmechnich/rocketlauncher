BINARY = rocketlauncher
PREFIX = /usr/local/bin

OBJECTS = main.o
HEADERS = \
	Command.hh \
	Common.hh \
	CursesInterface.hh \
	IOKitInterface.hh \
	Launcher.hh \
	Launcher.icc \
	LibUSBInterface.hh \
	LibUSB10Interface.hh
EXTRA_FILES = Makefile 81-rocket.rules

# configuration
$(info **  Configuration Stage)

# override libusb from command line
ifdef USE_LIBUSB
$(info Forcing USE_LIBUSB=$(USE_LIBUSB))
endif

# get default target from previous build, by default build release
DEBUG := $(shell !(test -e .stamp-build.debug); echo $$?)
ifeq ($(DEBUG),1)
DEFAULTTARGET=debug
else
DEFAULTTARGET=release
endif

# set DEBUG from make target
ifeq ($(MAKECMDGOALS),release)
DEBUG = 0
endif
ifeq ($(MAKECMDGOALS),debug)
DEBUG = 1
endif

# set compiler flags
ifeq ($(DEBUG),1)
CXXFLAGS += -g -DDEBUG
else
CXXFLAGS ?= -O2
endif

# default flags
CXXFLAGS += -Wall
LDFLAGS  += -lncurses

# set libusb version from previous build, overridden by USE_LIBUSB
STAMP_LIBUSB := $(shell ls -1 .stamp-deps.* 2>/dev/null | head -n1 | cut -d. -f3-)
USE_LIBUSB ?= $(STAMP_LIBUSB)
ifneq (x$(STAMP_LIBUSB),x)
$(info Previous build uses $(STAMP_LIBUSB))
endif

# auto-detect library if not already set
$(info Detecting installed libraries)
HAVE_LIBUSB   := $(shell !(pkg-config --exists libusb); echo $$?)
HAVE_LIBUSB10 := $(shell !(pkg-config --exists libusb-1.0); echo $$?)
HAVE_IOKIT    := $(shell !(test x`uname -s` = xDarwin); echo $$?)
ifeq ($(HAVE_LIBUSB),1)
AVAIL_LIBUSB += libusb
USE_LIBUSB ?= libusb
endif
ifeq ($(HAVE_LIBUSB10),1)
AVAIL_LIBUSB += libusb-1.0
USE_LIBUSB ?= libusb-1.0
endif
ifeq ($(HAVE_IOKIT),1)
AVAIL_LIBUSB += IOKit
USE_LIBUSB ?= IOKit
endif
ifdef AVAIL_LIBUSB
$(info Found: $(AVAIL_LIBUSB))
endif
ifndef USE_LIBUSB
$(error No usb library found, aborting)
endif

# updated flags
ifeq ($(USE_LIBUSB),libusb)
CXXFLAGS += `pkg-config --cflags libusb` -DHAVE_LIBUSB
LDFLAGS  += `pkg-config --libs libusb`
endif
ifeq ($(USE_LIBUSB),libusb-1.0)
CXXFLAGS += `pkg-config --cflags libusb-1.0` -DHAVE_LIBUSB10
LDFLAGS  += `pkg-config --libs libusb-1.0`
endif
ifeq ($(USE_LIBUSB),IOKit)
CXXFLAGS += -DHAVE_IOKIT
LDFLAGS  += -framework IOKit -framework CoreFoundation
OBJECTS  += IOKitInterface.o
endif

# build rules
$(info **  Build Stage)

all: $(DEFAULTTARGET)
	@echo "Choose USB library with e.g. 'USE_LIBUSB=libusb-1.0 make'"
	@echo "Force release build with 'make release'"
	@echo "Force debug   build with 'make debug'"

all-avail:
	for i in $(AVAIL_LIBUSB); do \
	  rm -f $(OBJECTS); USE_LIBUSB=$$i $(MAKE) debug; mv $(BINARY) $(BINARY)-$$i; \
	done

release: .stamp-build.release $(BINARY)
	@echo "Target '$@' built successfully"

debug: .stamp-build.debug $(BINARY)
	@test -e errpipe || mkfifo errpipe
	@echo "Target '$@' built successfully"
	@echo "Run as './$(BINARY) 2> errpipe' and read with"
	@echo "  e.g. 'tail -f errpipe' in another terminal"

.stamp-build.%:
	@rm -f $(BINARY)
	@rm -f .stamp-build.*
	@touch $@

.stamp-deps.$(USE_LIBUSB):
	@rm -f .stamp-deps.*
	@touch .stamp-deps.$(USE_LIBUSB)

$(OBJECTS): %.o: %.cc $(HEADERS) .stamp-deps.$(USE_LIBUSB)
	g++ -c $(CXXFLAGS) -o $@ $<

$(BINARY): $(OBJECTS)
	g++ $(LDFLAGS) -o $@ $^

debuglib:
	cd ext && make

uninstall-debuglib:
	cd ext && make uninstall
	cd ext && make distclean

clean:
	rm -f  $(OBJECTS) $(BINARY) $(BINARY)-* *~
	rm -rf $(BINARY).dSYM
distclean: clean
	rm -f errpipe
	rm -f .stamp*

install: $(BINARY)
	install -s -m 755 $(BINARY) $(PREFIX)
install-udev:
	install -m 644 81-rocket.rules /etc/udev/rules.d
uninstall:
	rm -f $(PREFIX)/$(BINARY)
	rm -f /etc/udev/rules.d/81-rocket.rules

DIST_FILES   = $(MAIN) $(HEADERS) $(EXTRA_FILES)
dist:
	rm -rf .dist.tmp
	mkdir -p .dist.tmp/$(BINARY)
	cp $(DIST_FILES) .dist.tmp/$(BINARY)
	(cd .dist.tmp && tar zcvf ../$(BINARY).tar.gz $(BINARY))
	rm -rf .dist.tmp
