# $Id$ 

pch:
	cd include && $(MAKE) $(AM_MAKEFLAGS) pch

SUBDIRS = include common modules server client util etc dat debug

if PREFIX_IS_PWD
else
EXTRA_DIST = softbot showps.sh etc dat

install-exec-local:
	install -c softbot $(prefix)
	install -c showps.sh $(prefix)

uninstall-local:
	rm -f $(prefix)/softbot
	rm -f $(prefix)/showps.sh
endif
