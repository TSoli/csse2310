#/bin/bash
# wait_until_listening_on_port.sh pid port-num 
# Exits when the given process is listening on the given port or has exited
# (or we have waited 3 seconds)
# If the port number is not given then we wait until listening on any port
if [ ! "$1" ] ; then
    echo "Usage: $0 pid port-num" >&2
    exit 1
fi
pid="$1"

# Steps
# - file descriptors the process has
# - filter to sockets
# - extract inode
# - look up inode in /proc/self/net/tcp
# - filter to listening sockets only
# - extract the port number being listened on (hex)
# - convert to decimal
port="$2"
hexport=$(printf %04X $port)

declare -i count_iterations=0
while [ $count_iterations -lt 20 ] ; do
    if ! ps -p$pid > /dev/null ; then
	# Process does not exist - abort
	exit 1
    fi
    sleep 0.15
    listening_ports=$(ls -l /proc/$pid/fd 2>/dev/null | grep socket | cut -d "[" -f 2 | tr -d ] | xargs -I X grep X /proc/self/net/tcp | grep "00000000:0000 0A" | cut -c 16-19 | xargs -I X printf %d\\n 0xX)
    if [ "$listening_ports" ] ; then
	if [ "$port" ] ; then
	    # Looking for specific port
	    if echo "$listening_ports" | grep "$port" >&/dev/null ; then
		# Process is listening on given port
		exit 0
	    fi
	    # else process is listening, but not on the right port - keep trying
	else 
	    # Not looking for specific port - but process is listening
	    exit 0
	fi
    fi
    # else process exists but is not listening
    count_iterations+=1
done
exit 1
