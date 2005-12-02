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
	@echo " iconv-install - optional"
	@echo " readline-install - optional"
	@echo " ncurses-install - optional"
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

###############################################################################
expat: expat-export
	cd expat && make

koma2c: koma2c-export
	cd koma2c && make

iconv: iconv-export
	cd libiconv-1.9.1 && make

readline: readline-export
	cd readline-5.0 && make

ncurses: ncurses-export
	cd ncurses-5.5 && make

###############################################################################
expat-install: expat
	cd expat && make install

koma2c-install: koma2c
	cd koma2c && make install

iconv-install: iconv
	cd libiconv-1.9.1 && make install

readline-install: readline
	cd readline-5.0 && make install

ncurses-install: ncurses
	cd ncurses-5.5 && make install

###############################################################################
expat-clean:
	rm -rf expat

koma2c-clean:
	rm -rf koma2c

iconv-clean:
	rm -rf libiconv-1.9.1

readline-clean:
	rm -rf readline-5.0

ncurses-clean:
	rm -rf ncurses-5.5

###############################################################################
test:
	-echo "Not Supported Yet"
	-echo "Who's gonna add 'test' feature??"

