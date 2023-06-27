#!/bin/bash
# check_crackserver_invalid_command command ...
# This test sends the commands on the command line (with a 0.5 second delay
# after each command). Single client is used.

# Determine a free port for the server to listen on
port=$(testfiles/freeport.sh)

rm -f /tmp/stderr.$$ /tmp/client.$$.out

# Start up the server being tested in the background and wait for it to
# be listening
${crackserver:=./crackserver} --port $port 2>/tmp/stderr.$$ >/dev/null &
server_pid=$!
testfiles/wait_until_listening.sh $server_pid $port

# Use nc to send the specified commands to the server. 
(
    for i in "$@" ; do
	echo "$i"
	sleep 0.5
    done
) | nc -4 localhost $port > /tmp/client.$$.out &
client_pid=$!

# Wait for 1.5 seconds
sleep 1.5
# Kill off the client and the server
kill $client_pid >&/dev/null
if ! wait $client_pid >&/dev/null ; then
    echo "Error - demo-crackclient exited abnormally" >&2
fi
kill -TERM $server_pid >&/dev/null || kill -KILL $server_pid >&/dev/null
wait $server_pid >&/dev/null

# Print out the response the server sent back to the client
# and output the server's stderr to stderr (minus the line that mentions the 
# port number (first line)).
echo Got response from crackserver:
cat /tmp/client.$$.out
grep -v $port /tmp/stderr.$$ >&2
rm -f /tmp/client.$$.out /tmp/stderr.$$
exit 0
