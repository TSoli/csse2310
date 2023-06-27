#!/bin/bash
# Check that crackserver reports the correct port number to stderr
# If an argument is given, it must be a port number to listen on (may be 0)
# If no port number is given, we choose one randomly.
# If the name "NONE" is given then we don't supply this arg to crackserver
 
rm -f /tmp/stderr

if [ "$1" = "NONE" ] ; then
    port=""
elif [ "$1" ] ; then
    # Port number given on the command line
    port=$1
else
    # Choose a random free port
    port=$(testfiles/freeport.sh)
    sleep 0.5
fi

# Start up crackserver in the background and wait for it to be listening
if [ "${port}" ] ; then
    ${crackserver:=./crackserver} --port ${port} 2>/tmp/stderr &
    server_pid=$!
else
    # No port given
    ${crackserver:=./crackserver} 2>/tmp/stderr &
    server_pid=$!
    # Change port value to 0 so following code works
    port=0
fi
testfiles/wait_until_listening.sh "$server_pid" "$port"

# Open the crackserver's stderr for reading (on fd 4)
exec 4</tmp/stderr
# Read the first line from that stderr (should contain a port number)
read reported_port <&4

if [ "$port" != 0 ] ; then
    # If we asked for a specific port, make sure that number was output
    if ! echo "$reported_port" | grep "$port" >&/dev/null || [ "$port" -ne "${reported_port}" ] 2>/dev/null; then
	echo "Reported port incorrect - expected $port got $reported_port" >&2
	status=1
    else
	echo "Reported port OK" >&2
	status=0
    fi
else 
    # We asked for port zero - just make sure we got a number - we delete
    # all the digits in the response and make sure nothing is left
    p=$(echo $reported_port | tr -d 0-9)
    if [ ! "$p" ] ; then
	echo "Reported port is numeric" >&2
	status=0
    else
	echo "Reported port non numeric - got $reported_port" >&2
	status=2
    fi
fi

if [ "$status" = 0 ] ; then 
    # If the reported port number was OK, see if netcat can connect
    # We start netcat in verbose mode so it reports a connection message if
    # it connects.
    rm -f /tmp/stderr2
    timeout 1.2 nc -4 -v localhost $reported_port >/dev/null 2>/tmp/stderr2
    if grep "Connected to 127.0.0.1" /tmp/stderr2 >&/dev/null ; then
	echo "Test client connected to crackserver"
    else
	echo "Test client failed to connect to crackserver"
	status=3
    fi
fi

# Kill off the server under test
kill -9 $server_pid >&/dev/null
wait $server_pid >&/dev/null

# Output crackserver's standard error to standard error here (minus the first line)
tail -n +2 /tmp/stderr >&2
rm -f /tmp/stderr*

exit "$status"
