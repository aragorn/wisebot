#!/bin/sh
#$Id$

set -x
dir=`pwd`
reldir=`basename $dir`
if [ "$reldir" = "dat" ]
then
	cd lexicon && rm -f * ; cd $dir
	cd lexicon/wordtoid && rm -f * ; cd $dir
	cd lexicon/idtoword && rm -f * ; cd $dir
	cd indexer && rm -f * ; cd $dir
else
	echo "you must run this script in dat directory"
fi

../scripts/clear_ipcs
