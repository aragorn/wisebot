#!/bin/sh
#$Id$

set -x
dir=`pwd`
reldir=`basename $dir`
if [ ! "$reldir" = "dat" -a ! "$reldir" = "var" ]
then
	echo "you must run this script in dat directory"
	exit
fi

subdirs="lexicon indexer";
for dir in $subdirs;
do
	test -d $dir && rm -f $dir/* ;
done

test -d ../logs && rm -f ../logs/indexer_log* 
test -x ../scripts/clear_ipcs && ../scripts/clear_ipcs

echo "done."
