# $Id$ 
# Generated automatically from Makefile.in by configure.
#
# build extra libraries for softbot
#
# by aragorn, 2002/09/30
#

prefix = $(shell cd .. && pwd)
exec_prefix = ${prefix}
###############################################################################
all: 
	@echo "Choose which package to install."
	@echo "available packages:"
	@echo " expat-install  - optional"
	@echo " koma2c-install - required"
	@echo " daum_koma-install - optional"
	@echo " iconv-install - optional"
	@echo " readline-install - optional"
	@echo " ncurses-install - optional"
	@echo " apr-install - optional"
	@echo " apr-util-install - optional"
	@echo "ex) make koma2c-install"
###############################################################################
cvs-export: expat-export koma2c-export
build: expat koma2c
install: expat-install koma2c-install
clean: expat-clean koma2c-clean
###############################################################################
expat-export:
	[ -f expat/configure ] || (gzip -c -d expat.tar.gz | tar xf -)
	cd expat && \
	./configure --prefix=$(prefix) --includedir=$(prefix)/include/expat

koma2c-export:
	[ -f koma2c/buildconf ] || (cvs co koma2c)
	cd koma2c && \
	./buildconf && \
	./configure --prefix=$(prefix) --includedir=$(prefix)/include/koma

daum_koma-export:
	[ -f daum_koma/bootstrap ] || (gzip -c -d daum_koma.tar.gz | tar xf -)
	cd daum_koma && \
	./bootstrap && \
	./configure --prefix=$(prefix) --includedir=$(prefix)/include/daum_koma

iconv-export:
	[ -f libiconv-1.9.1/configure ] || (gzip -c -d libiconv-1.9.1.tar.gz | tar xf -)
	cd libiconv-1.9.1 && \
	./configure --prefix=$(prefix) --includedir=$(prefix)/include/iconv

readline-export:
	[ -f readline-5.0/configure ] || (gzip -c -d readline-5.0.tar.gz | tar xf -)
	cd readline-5.0 && \
	./configure --prefix=$(prefix)

ncurses-export:
	[ -f ncurses-5.5/configure ] || (gzip -c -d ncurses-5.5.tar.gz | tar xf -)
	cd ncurses-5.5 && \
	./configure --prefix=$(prefix)

apr-export:
	[ -f apr/buildconf ] || (gzip -c -d apr.tar.gz | tar xf -)
	cd apr && \
	./buildconf && \
	./configure --prefix=$(prefix)

apr-util-export:
	[ -f apr-util/buildconf ] || (gzip -c -d apr-util.tar.gz | tar xf -)
	cd apr-util && \
	./buildconf && \
	./configure --prefix=$(prefix) --with-apr=$(shell pwd)/apr

###############################################################################
expat: expat-export
	cd expat && make

koma2c: koma2c-export
	cd koma2c && make

daum_koma: daum_koma-export
	cd daum_koma && make

iconv: iconv-export
	cd libiconv-1.9.1 && make

readline: readline-export
	cd readline-5.0 && make

ncurses: ncurses-export
	cd ncurses-5.5 && make

apr: apr-export
	cd apr && make

apr-util: apr-util-export
	cd apr-util && make

###############################################################################
expat-install: expat
	cd expat && make install

koma2c-install: koma2c
	cd koma2c && make install

daum_koma-install: daum_koma
	cd daum_koma && make install

iconv-install: iconv
	cd libiconv-1.9.1 && make install

readline-install: readline
	cd readline-5.0 && make install

ncurses-install: ncurses
	cd ncurses-5.5 && make install

apr-install: apr
	cd apr && make install

apr-util-install: apr-util
	cd apr-util && make install
###############################################################################
expat-clean:
	rm -rf expat

koma2c-clean:
	rm -rf koma2c

daum_koma-clean:
	rm -rf daum_koma

iconv-clean:
	rm -rf libiconv-1.9.1

readline-clean:
	rm -rf readline-5.0

ncurses-clean:
	rm -rf ncurses-5.5

apr-clean:
	rm -rf apr 

apr-util-clean:
	rm -rf apr-util

###############################################################################
test:
	-echo "Not Supported Yet"
	-echo "Who's gonna add 'test' feature??"

