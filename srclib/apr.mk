# $Id$ 

apr_name = apr-1.2.12
apu_name = apr-util-1.2.12

apr-default:
	@echo Available targets: apr-install apr-util-install

apr-export:
	[ -f $(apr_name)/buildconf ] || (gzip -c -d $(apr_name).tar.gz | tar xf -)
	cd $(apr_name) && \
	./configure --prefix=$(prefix)

apr-util-export:
	[ -f $(apu_name)/buildconf ] || (gzip -c -d $(apu_name).tar.gz | tar xf -)
	cd $(apu_name) && \
	./configure --prefix=$(prefix) --with-apr=$(shell pwd)/$(apr_name)

apr: apr-export
	cd $(apr_name) && make

apr-util: apr-util-export
	cd $(apu_name) && make

apr-install: apr
	cd $(apr_name) && make install

apr-util-install: apr-util
	cd $(apu_name) && make install

apr-clean:
	rm -rf $(apr_name)

apr-util-clean:
	rm -rf $(apu_name)

