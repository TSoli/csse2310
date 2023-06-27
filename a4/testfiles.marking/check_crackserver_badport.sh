#!/bin/bash
# Check that crackserver reports being unable to listen on an already occupied 
# port

rm -f /tmp/stderr

port=$(testfiles/freeport.sh)

# Start a dummy server listening on this port in the background
nc --no-shutdown -4 -l ${port} >&/dev/null </dev/null &
nc_pid=$!
sleep 1

# Start up crackserver and try to listen on this port
timeout 3 ${crackserver:=./crackserver} --port ${port} 
status=$?

# Kill off dummy server
kill -9 $nc_pid >&/dev/null
wait $nc_pid >&/dev/null

exit $status
