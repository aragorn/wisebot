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
all: install
cvs-export: expat-export koma2c-export
build: expat koma2c bdb
install: expat-install koma2c-install
clean: expat-clean koma2c-clean bdb-clean

# XXX we don't need w3c-libwww & xmlrpc for supreme-release

#cvs-export: apr-export apr-util-export expat-export w3c-libwww-export xmlrpc-c-export
#build: apr apr-util expat w3c-libwww xmlrpc-c
#install: apr-install apr-util-install expat-install w3c-libwww-install xmlrpc-c-install
#clean: apr-clean apr-util-clean expat-clean w3c-libwww-clean xmlrpc-c-clean
###############################################################################
apr-export:
	[ -f apr/buildconf ] || (gzip -c -d apr.tar.gz | tar xf -)
	cd apr && \
	./buildconf && \
	./configure --prefix=$(prefix)

apr-util-export:
	[ -f apr-util/buildconf ] || (gzip -c -d apr-util.tar.gz | tar xf -)
	cd apr-util && \
	./buildconf && \
	./configure --prefix=$(prefix) --with-apr=../apr

expat-export:
	[ -f expat/configure ] || (gzip -c -d expat.tar.gz | tar xf -)
	cd expat && \
	./configure --prefix=$(prefix)

bdb-export:
	[ -f db-4.3.27.NC/dist/configure ] || (gzip -c -d db-4.3.27.NC.tar.gz | tar xf -)
	cd db-4.3.27.NC/build_unix && \
	../dist/configure --prefix=$(prefix)

sn2-export:
	[ -f SynapNext2/configure ] || (gzip -c -d SynapNext2.tar.gz | tar xf -)
	cd SynapNext2 && \
	./buildconf && \
	./configure --prefix=$(prefix)

koma2c-export:
	[ -f koma2c/buildconf ] || (cvs export -D today koma2c)
	cd koma2c && \
	./buildconf && \
	./configure --prefix=$(prefix)
###############################################################################
apr: apr-export
	cd apr && make

apr-util: apr-util-export
	cd apr-util && make

expat: expat-export
	cd expat && make

bdb: bdb-export
	cd db-4.3.27.NC/build_unix && make

sn2: sn2-export
	cd SynapNext2 && make

koma2c: koma2c-export
	cd koma2c && make
###############################################################################
apr-install: apr
	cd apr && make install

apr-util-install: apr-util
	cd apr-util && make install

expat-install: expat
	cd expat && make install

bdb-install: bdb
	cd db-4.3.27.NC/build_unix && make install

sn2-install: sn2
	cd SynapNext2 && make install

koma2c-install: koma2c
	cd koma2c && make install
###############################################################################
apr-clean:
	rm -rf apr

apr-util-clean:
	rm -rf apr-util

expat-clean:
	rm -rf expat

bdb-clean:
	rm -rf db-4.3.27.NC

sn2-clean:
	rm -rf SynapNext2

koma2c-clean:
	rm -rf koma2c
###############################################################################
test:
	-echo "Not Supported Yet"
	-echo "Who's gonna add 'test' feature??"

