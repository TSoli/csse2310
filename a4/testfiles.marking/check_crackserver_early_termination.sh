#!/bin/bash
# check_crackserver_early_termination.sh num-threads cipher-text
# The cipher-text is expected to be found early in the relevant section of
# the dictionary. Overall we should have very few crypt calls - we test for
# fewer than 100,000 which should allow an appropriate margin.

# Usage: count_active_threads pid basecount
function count_active_threads() {
    numthreads=$(ps -L -p$1 | grep $1 | wc -l)
    basecount=$2
    echo $((numthreads - basecount))
}

# Requires use of thread_interposer
# Usage: count_total_threads basecount
function count_total_threads() {
    numthreads=$(cat /tmp/threadcount)
    basecount=$1
    echo $((numthreads - basecount))
}

trap cleanup EXIT

function cleanup() {
    terminate_processes
    rm -f /tmp/client.$$.* /tmp/server.out.$$ /tmp/server.err.$$ 
}

function terminate_processes() {
    if [ "$server_pid" ] ; then
	if [ "$debug" ] ; then
	    echo "Killing off crackserver (pid $server_pid)" >&2
	fi
	kill -TERM $server_pid >&/dev/null || kill -KILL $server_pid >&/dev/null
	wait $server_pid >&/dev/null
	unset server_pid
    fi
}

# Determine a free port for the server to listen on
port=$(testfiles/freeport.sh)

# Start up the server being tested in the background and wait for it to
# be listening. We remove temporary files created by interposer
rm -f /tmp/listen.* /tmp/cryptcount /tmp/threadcount /tmp/termination.report
LD_PRELOAD=testfiles/thread_interposer.so ${crackserver:=./crackserver} --port $port --dictionary testfiles/large.dict  >/tmp/server.out.$$ 2>/tmp/server.err.$$ &
server_pid=$!

# Wait until server is listening (wait for appearance of temporary file
# created by listen interposer)
iterations=0
while [ ! -f /tmp/listen.$server_pid ] ; do
    sleep 0.1
    if [ $((iterations++)) -gt 30 ] ; then
	if ps -p $server_pid > /dev/null ; then 
	    echo "Server is dead - aborting test" >&2
	else
	    echo "Server is alive but did not listen - aborting test" >&2
	fi
	exit 1
    fi
done

baseactivethreadcount=$(count_active_threads $server_pid 0)
basetotalthreadcount=$(count_total_threads 0)

# Headers for our thread count files (ensures previous data wiped also).
rm -f /tmp/totalthreadcount.txt /tmp/activethreadcount.txt /tmp/client.$$.out
echo "Thread counts reported before each request line. (0 assumed before any clients created.)" > /tmp/activethreadcount.txt
echo "Total threads created since listen" > /tmp/totalthreadcount.txt

# Send decryption request
echo "crack $2 $1" | demo-crackclient $port > /tmp/client.$$.out
# Send crypt request - this ensures our crypt count is updated on connect
echo "crypt chicken aa" | demo-crackclient $port >> /tmp/client.$$.out

threadcount=$(count_active_threads $server_pid $baseactivethreadcount)
echo End: $threadcount >> /tmp/activethreadcount.txt
if [ "$debug" ] ; then
    echo "Additional threads: $threadcount" >&2
fi
threadcount=$(count_total_threads $basetotalthreadcount)
echo $threadcount >> /tmp/totalthreadcount.txt
if [ "$debug" ] ; then
    echo "Total threads since listen: $threadcount" >&2
fi


# Have now completed the operations - kill off processes and remove pipes
terminate_processes

if [ -s /tmp/server.out.$$ ] ; then
    echo "Output from crackserver is:"
    echo "------------------------"
    cat /tmp/server.out.$$
    echo "------------------------"
fi

# Send the output from the clients to stdout
echo "Output from clients is:"
echo "------------------------"
cat /tmp/client.$$.out
echo "------------------------"

# Send crackserver's stderr to stderr (excluding the line with the 
# port number on it)
grep -v ${port} /tmp/server.err.$$ >&2
# Check crypt count is < 100000
cryptcount=$(cat /tmp/cryptcount)
if [ "${cryptcount}" -lt 100000 ] ; then
    echo "crypt count less than 100000 - as expected" >&2
else
    echo "crypt count more than expected ($cryptcount)" >&2
fi

# Remove any temporary files
rm -f /tmp/client.$$.* /tmp/server.out.$$ /tmp/server.err.$$ /tmp/listen.*
rm -f /tmp/cryptcount 
exit 0
