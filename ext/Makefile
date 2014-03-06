LIBS = libusb-1.0.9 libusb-compat-0.1.4

# install libusb-1.0 before configuring/building libusb-compat
.conf-stamp.libusb-compat-0.1.4: .install-stamp.libusb-1.0.9

# try first without, then with sudo
mysudo = ($(1) || sudo $(1))

all: build
	$(MAKE) install

$(LIBS): 
	tar jxvf $@.tar.bz2

.conf-stamp.%: %
	cd `echo $@ | cut -d. -f3-` && ./configure --enable-debug-log
	touch $@

.build-stamp.%: .conf-stamp.%
	cd `echo $@ | cut -d. -f3-` && make
	touch $@

.install-stamp.%: .build-stamp.%
	cd `echo $@ | cut -d. -f3-` && $(call mysudo,make install)
	touch $@

.PRECIOUS: .conf-stamp.% .build-stamp.% .install-stamp.%

build:   $(patsubst %,.build-stamp.%,$(LIBS))
install: $(patsubst %,.install-stamp.%,$(LIBS))

uninstall:
	for i in `ls -1 .install-stamp.* 2>/dev/null`; do \
	(cd `echo $$i | cut -d. -f3-` && $(call mysudo,make uninstall)) && \
	rm -f $$i; done
	-test -d /usr/local/include/libusb-1.0 && \
	$(call mysudo,rmdir /usr/local/include/libusb-1.0 2>/dev/null)

clean:
	-for i in $(LIBS); do \
	(test -e $$i/Makefile && cd $$i && make clean); done
	rm -f .build-stamp.*

distclean: clean
	rm -rf .install-stamp.*
	rm -rf .conf-stamp.*
	rm -rf $(LIBS)
	rm -f *~
