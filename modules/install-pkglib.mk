# $Id$

install-pkglibLTLIBRARIES: $(pkglib_LTLIBRARIES)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(pkglibdir)
	@list='$(pkglib_LTLIBRARIES)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "$(top_srcdir)/build/instdso.sh SH_LIBTOOL='$(LIBTOOL)' $$p $(DESTDIR)$(pkglibdir)"; \
	    $(top_srcdir)/build/instdso.sh SH_LIBTOOL='$(LIBTOOL)' $$p $(DESTDIR)$(pkglibdir); \
	  else :; fi; \
	done;

#	$(top_srcdir)/build/instdso.sh SH_LIBTOOL='$(LIBTOOL)' $$p $(DESTDIR)$(pkglibdir);
#	$(LIBTOOL) --mode=install $(INSTALL) $(INSTALL_STRIP_FLAG) $$p $(DESTDIR)$(pkglibdir)/$$p; 
