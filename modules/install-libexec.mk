# $Id$

install-libexecLTLIBRARIES: $(libexec_LTLIBRARIES)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libexecdir)
	@list='$(libexec_LTLIBRARIES)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "$(top_srcdir)/build/instdso.sh SH_LIBTOOL='$(LIBTOOL)' $$p $(DESTDIR)$(libexecdir)"; \
	    $(top_srcdir)/build/instdso.sh SH_LIBTOOL='$(LIBTOOL)' $$p $(DESTDIR)$(libexecdir); \
	  else :; fi; \
	done;


#	    $(top_srcdir)/build/instdso.sh SH_LIBTOOL='$(LIBTOOL)' $$p $(DESTDIR)$(libexecdir);
#	    $(LIBTOOL) --mode=install $(INSTALL) $(INSTALL_STRIP_FLAG) $$p $(DESTDIR)$(libexecdir)/$$p; 
