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

bdb-export:
	[ -f db-4.3.27.NC/dist/configure ] || (gzip -c -d db-4.3.27.NC.tar.gz | tar xf -)
	cd db-4.3.27.NC/build_unix && \
	../dist/configure --prefix=$(prefix) --includedir=$(prefix)/include/db

sn2-export:
	[ -f SynapNext2/configure ] || (gzip -c -d SynapNext2.tar.gz | tar xf -)
	cd SynapNext2 && \
	./buildconf && \
	./configure --prefix=$(prefix) --includedir=$(prefix)/include/sn2

koma2c-export:
	[ -f koma2c/buildconf ] || (cvs co koma2c)
	cd koma2c && \
	./buildconf && \
	./configure --prefix=$(prefix) --includedir=$(prefix)/include/koma
###############################################################################
expat: expat-export
	cd expat && make

bdb: bdb-export
	cd db-4.3.27.NC/build_unix && make

sn2: sn2-export
	cd SynapNext2 && make

koma2c: koma2c-export
	cd koma2c && make
###############################################################################
expat-install: expat
	cd expat && make install

bdb-install: bdb
	cd db-4.3.27.NC/build_unix && make install

sn2-install: sn2
	cd SynapNext2 && make install

koma2c-install: koma2c
	cd koma2c && make install
###############################################################################
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

