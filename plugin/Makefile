ROOT = $(shell pwd)
INSTALLDIR ?= $(ROOT)/../../sliderule/stage/sliderule

default:
	make -j8 -C build

config:
	mkdir -p build
	cd build && cmake -D CMAKE_BUILD_TYPE=Release .. -D INSTALLDIR=$(INSTALLDIR)

install:
	make -C build install

publish:
	aws s3 cp build/coastnet.so s3://sliderule/config/
	
publish-local:
	cp build/coastnet.so /plugin/

distclean:
	rm -Rf build