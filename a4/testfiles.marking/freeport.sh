#!/bin/bash
declare -i port
port=$RANDOM
port+=1024
while true ; do
    rm -f /tmp/nc.out
    nc -v -4 -l ${port} > /tmp/nc.out 2>&1 </dev/null &
    sleep 0.2
    nc_pid=$!
    exec 4</tmp/nc.out
    read line <&4
    read line <&4
    kill $nc_pid >&/dev/null
    wait $nc_pid >&/dev/null
    if echo $line | grep Listening &>/dev/null ; then
	break
    else
	port+=1
    fi
done
rm -f /tmp/nc.out
echo $port
