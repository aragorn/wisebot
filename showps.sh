#!/bin/sh

#perl -e "while(1) { system('clear'); system('scripts/view_registry dat/softbotd.lg.reg'); system('./softbot status'); sleep(3);}"
while [ 1 ]  
do
    clear
    scripts/view_registry dat/softbotd.reg
    ./softbot status
    sleep 3
done

