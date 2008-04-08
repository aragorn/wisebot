# $Id$ 

pch:
	cd include && $(MAKE) $(AM_MAKEFLAGS) pch

SUBDIRS = include common modules server client util etc debug

#auxdir = @ac_aux_dir@
#AUX_DIST = $(auxdir)/install-sh $(auxdir)/missing \
#		   $(auxdir)/mkinstalldirs $(auxdir)/depcomp
#CONFIG_OUTPUTS = $(auxdir)/config.guess $(auxdir)/config.sub \
#				 config.status config.log config.cache 
#ACLOCAL = aclocal -I $(auxdir) 
#AUTOMAKE_OPTIONS = foreign
#
#MAINTAINERCLEANFILES = Makefile Makefile.in Make.defs aclocal.m4 configure \
#		include/auto_config.h.in include/auto_config.h \
#		include/stamp-h.in  include/stamp-h1 include/stamp-h\
#		$(AUX_DIST) $(CONFIG_OUTPUTS) \
#		lib/* lib/softbot*/* \
#		bin/*

# dist_pkgdata_DATA = srclib/Makefile.in
EXTRA_DIST = softbot showps.sh etc dat

install-exec-local:
	install -c softbot $(prefix)
	install -c showps.sh $(prefix)

uninstall-local:
	rm -f $(prefix)/softbot
	rm -f $(prefix)/showps.sh
