#!/bin/sh
#$Id$

set -x

SOFTBOTD_PID_FILE=../logs/softbotd.pid
PS=ps

is_running()
{
    # RETURN VALUE
    # > 0 : PID of the process
    #   0 : is not running
    #  -1 : empty pid file
    #  -2 : no pid file
    local PID_FILE=$1

    if [ ! -r "$PID_FILE" ]; then
        # no pid file
        echo "-2"
        return -2
    fi

    local PID=`cat $PID_FILE 2>/dev/null`

    if [ ! -n "$PID" ]; then
        # empty pid file
        echo "-1"
        return -1
    fi

    kill -0 $PID > /dev/null 2>&1

    if [ $? != "0" ]; then
        # no such process
        echo "0"
        return 0
    fi

    echo "$PID"
    return 1
}

PID=`is_running "$SOFTBOTD_PID_FILE"`
(( PID > 0 )) && printf "[ERROR: softbotd process is running($PID)] " && exit 1

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
