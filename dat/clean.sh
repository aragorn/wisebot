#!/bin/sh
#$Id$

set -x
dir=`pwd`
reldir=`basename $dir`
if [ "$reldir" = "dat" ]
then
	cd test && rm -f * ; cd $dir
	cd cdm && rm -f * ; cd $dir
	cd did && rm -f * ; cd $dir
	cd lexicon && rm -f * ; cd $dir
	cd lexicon/wordtoid && rm -f *; cd $dir
	cd lexicon/idtoword && rm -f *; cd $dir
	cd indexer && rm -f * ; cd $dir
	cd vrf && rm -f * ; cd $dir
	cd ../logs && rm -f error_log softbotd.pid ; cd $dir
	rm -f softbotd*.reg ; cd $dir
	cd crawler && rm -f * ; cd $dir
else
	echo "you must run this script in dat directory"
fi

../scripts/clear_ipcs
