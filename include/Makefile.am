# $Id$

pch: $(noinst_HEADERS)
	rm -f *.gch
	$(CC) $(DEFS) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -c -o common_core.h.gch common_core.h
	$(CC) $(DEFS) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -c -o common_util.h.gch common_util.h

dist_pkgdata_DATA = auto_config.h.in
if PREFIX_IS_PWD
noinst_HEADERS  = config.h constants.h hook.h ipc.h log_error.h modules.h \
                  registry.h softbot.h util.h memory.h \
                  common_core.h common_util.h
else
include_HEADERS = config.h constants.h hook.h ipc.h log_error.h modules.h \
                  registry.h softbot.h util.h memory.h \
                  common_core.h common_util.h
endif

