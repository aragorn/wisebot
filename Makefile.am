# $Id$ 

auxdir = @ac_aux_dir@
AUX_DIST = $(auxdir)/install-sh $(auxdir)/missing \
		   $(auxdir)/mkinstalldirs $(auxdir)/depcomp
CONFIG_OUTPUTS = $(auxdir)/config.guess $(auxdir)/config.sub \
				 config.status config.log config.cache 
AUTOMAKE_OPTIONS = foreign
SUBDIRS = platform modules server client util debug include

MAINTAINERCLEANFILES = Makefile Makefile.in Make.defs aclocal.m4 configure \
		include/auto_config.h.in include/auto_config.h \
		include/stamp-h.in  include/stamp-h1 include/stamp-h\
		$(AUX_DIST) $(CONFIG_OUTPUTS) \
		lib/* libexec/* \
		bin/*

ACLOCAL = aclocal -I $(auxdir) 

# dist_pkgdata_DATA = srclib/Makefile.in
EXTRA_DIST = etc dat
dist-hook:
	rm -rf `find $(distdir)/ -name CVS`
	rm -rf `find $(distdir)/dat -name "*.txt"`
