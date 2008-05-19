# $Id$
dist_localstate_DATA  = clean.sh clean_index.sh

install-data-am:
	cd $(localstatedir) && chmod a+x clean.sh clean_index.sh
	mkdir -p $(localstatedir)/cdm
	mkdir -p $(localstatedir)/did
	mkdir -p $(localstatedir)/indexer
	mkdir -p $(localstatedir)/lexicon
