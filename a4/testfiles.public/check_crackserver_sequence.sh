#!/bin/bash
# check_crackserver_sequence.sh [--debug] [--dictionary file] [--maxconn n] \
#	    [--delay d] [--http] request-sequence-file
#
# Read lines from a file that describes a sequence of operations to be 
# carried out. Lines will have one of the following formats. Clients are 
# numbered from 1 (n below should be replaced by the client number).
#	# - line starting with # is a comment line and is ignored
#	sighup - SIGHUP signal is sent to the server
#	sigpipe - SIGPIPE signal is sent to the server
#	sleep duration - sleep for the given duration
#	n open - connection is opened for client n, but nothing is sent
#		(Note there is no need to open a connection first - sending
#		a message will open a connection if none is open)
#	n send message - send the given message from the given client. The
#		message is sent with "echo -e" so may contain backslash escape
#		sequences. The connection will be opened if one is not already 
#		open.
#	n sendnonewline message - as per send, but without the newline at the end
#	n close - close connection for client n (i.e. kill off client)
#	n read - read one line from client connection. (If a long delay is
#		expected or possible in client responses then this can be used
#		as a synchronisation mechanism - i.e. don't the messages to
#		the client get ahead of the responses
#	n readtimeout - attempt to read one line from client connection - with
#		0.5 second timeout. We expect to get the timeout, 
#		i.e. no data available.
#	n openhttp - open the HTTP port to the server. (This is only 
#		applicable if --http is specified on the command line. This
#		means we set A4_HTTP_PORT environment variable to a free port
#		before running the server.) Connections to the HTTP port
#		must be manually created using this command before use
#
# Options to this test program are:
#	--debug - print debugging information to standard error as we go
#	--dictionary file - if supplied start server with --dictionary file args
#	--maxconn n - if supplied start server with --maxconn n args (by 
#		default we used --maxconn 0)
#	--delay d - override the default delay between sequence actions (0.1s)
#	--http - see description above of openhttp
#
# The standard output of this test program is all of the data received by
# all of the clients (in sequence).
#
# The standard error of this test program is whatever is emitted by the server
# (minus the line containing the port number being listened on)
# (plus debug information if --debug is specified.)
#
# This program also captures data in the following files:
#	/tmp/activethreadcont.txt - just before reading every command in the
#		sequence file, and at the end of the sequence, we capture how
#		many threads are active in the server (ignoring those that
#		are active at the start).
#	/tmp/totalthreadcount.txt - the total number of threads that were
#		started by the server (ignoring those active at the start)
#	/tmp/cryptcount - total number of calls to crypt - updated at the
#		start of every thread (so may require a client to connect for
#		this value to be updated)
# The last two of these rely on the server being run with the 
# thread_interposer.so LD_PRELOAD (so we can capture appropriate data)
#	

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

nc_pids=()
clients=()

trap cleanup EXIT

function cleanup() {
    terminate_processes
    rm -f /tmp/client.$$.* /tmp/server.out.$$ /tmp/server.err.$$ 
}

function terminate_processes() {
    # Copy any remaining text from output pipes to the client's stdout file
    catpids=()
    for i in ${!nc_pids[@]} ; do
	if [ "$debug" ] ; then
	    echo "Consuming remaining data for client $i" >&2
	fi
	cat /tmp/client.$$.$i.pipe.out >> /tmp/client.$$.$i.stdout &
	catpids[$i]=$!
    done
    sleep ${delay}

    # Kill off the clients (if any) and server
    if [ ${#nc_pids[@]} -gt 0 ] ; then
	if [ "$debug" ] ; then
	    echo "Killing off clients (pids ${nc_pids[@]})" >&2
	fi
	kill -9 ${nc_pids[@]} >&/dev/null
	wait ${nc_pids[@]} >&/dev/null
	unset nc_pids
    fi
    if [ "$server_pid" ] ; then
	if [ "$debug" ] ; then
	    echo "Killing off crackserver (pid $server_pid)" >&2
	fi
	kill -9 $server_pid >&/dev/null
	wait $server_pid >&/dev/null
	unset server_pid
    fi

    # Wait for any remaining cat processes to finish (they should die without
    # needing to be killed)
    if [ ${#catpids[@]} -gt 0 ] ; then
	if [ "$debug" ] ; then
	    echo "Waiting for all client output to be saved" >&2
	fi
	wait ${catpids[@]} >&/dev/null
    fi

    # Remove the named pipes
    rm -f /tmp/client.$$.*.pipe.*
}

debug=""
dict=""
connlimit=0
delay=0.1
unset A4_HTTP_PORT httpport
while true ; do
    case "$1" in 
	--debug ) debug=1; shift 1;;
	--dictionary ) dict="$2"; shift 2;;
	--maxconn ) connlimit="$2" ; shift 2;;
	--delay ) delay="$2"; shift 2;;
	--http )
	    httpport=$(testfiles/freeport.sh)
	    export A4_HTTP_PORT=${httpport}
	    shift;;
	* ) break;
    esac
done

# Check sequence file exists
if [ ! -r "$1" ] ; then
    echo "No operation file provided" >&2
    exit 1
else
    # Read from fd 3 to retrieve operations
    exec 3< "$1"
fi

# Determine a free port for the server to listen on
port=$(testfiles/freeport.sh)
if [ "$debug" ] ; then
    echo "Identified free port number: $port" >&2
fi

# Start up the server being tested in the background and wait for it to
# be listening. We remove temporary files created by interposer
rm -f /tmp/listen.*
if [ "$debug" ] ; then
    echo "Starting crackserver in the background as follows:">&2
fi
if [ "${dict}" ] ; then
    if [ "$debug" ] ; then
	echo "LD_PRELOAD=testfiles/thread_interposer.so ./crackserver --maxconn $connlimit --port $port --dictionary $dict &"
    fi
    LD_PRELOAD=testfiles/thread_interposer.so ${crackserver:=./crackserver} --maxconn "$connlimit" --port $port --dictionary "$dict"  >/tmp/server.out.$$ 2>/tmp/server.err.$$ &
    server_pid=$!
else
    if [ "$debug" ] ; then
	echo "LD_PRELOAD=testfiles/thread_interposer.so ./crackserver --maxconn $connlimit --port $port &"
    fi
    LD_PRELOAD=testfiles/thread_interposer.so ${crackserver:=./crackserver} --maxconn "$connlimit" --port $port >/tmp/server.out.$$ 2>/tmp/server.err.$$ &
    server_pid=$!
fi

# Wait until server is listening (wait for appearance of temporary file
# created by listen interposer)
if [ "$debug" ] ; then
    echo "Started server - pid $server_pid" >&2
    echo "Waiting for server to listen ..." >&2
fi
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
if [ "$debug" ] ; then
    echo "Started crackserver on port $port." >&2
    echo "Active thread count = $baseactivethreadcount" >&2
    echo "Total thread count = $basetotalthreadcount" >&2
fi

# Headers for our thread count files (ensures previous data wiped also).
rm -f /tmp/totalthreadcount.txt /tmp/activethreadcount.txt
echo "Thread counts reported before each request line. (0 assumed before any clients created.)" > /tmp/activethreadcount.txt
echo "Total threads created since listen" > /tmp/totalthreadcount.txt

declare -i line=0;
# Read each line in the operations file
while read -r client request mesg <&3 ; do
    line+=1
    if [ "$debug" ] ; then
	echo "Got request line: $client $request $mesg" >&2
    fi
    if [[ $client =~ ^#.*$ ]] ; then
	# Skip over comments
	if [ "$debug" ] ; then
	    echo "Skipping comment" >&2
	fi
	continue;
    fi
    if [ "$client" = "sleep" ] ; then
	if [ "$debug" ] ; then
	    echo "Sleeping for ${request} seconds" >&2
	fi
	sleep ${request}
	# Skip everything else (e.g. we don't output a thread count)
	continue;
    fi
    # Output active thread count
    echo Line $line: $(count_active_threads $server_pid $baseactivethreadcount) >> /tmp/activethreadcount.txt
    if [ "${client}" = "sighup" ] ; then
	if [ "$debug" ] ; then
	    echo "Sending SIGHUP to server" >&2
	fi
	kill -HUP $server_pid
	sleep "${delay}"
	continue
    fi
    if [ "${client}" = "sigpipe" ] ; then
	if [ "$debug" ] ; then
	    echo "Sending SIGPIPE to server" >&2
	fi
	kill -PIPE $server_pid
	sleep "${delay}"
	continue
    fi
    # Work out the named pipe files for communicating with this client
    pipein=/tmp/client.$$.${client}.pipe.in
    pipeout=/tmp/client.$$.${client}.pipe.out
    if [ ! -p ${pipein} ] ; then
	# Input pipe doesn't exist (we assume the same is true of output pipe)
	# This means the client does not exist
	if [ "${request}" = "close" ] ; then
	    # The first time we've seen this client is with a close request
	    # - ignore this line
	    if [ "$debug" ] ; then
		echo "Ignoring close request for client we haven't sen before" >&2
	    fi
	    continue
	fi
	if [ "${request}" = "openhttp" ] ; then
	    if [ ! "${httpport}" ] ; then
		echo "HTTP port not enabled but request made to open HTTP port" >&2
		exit 1
	    fi
	    port_to_use=${httpport}
	else
	    port_to_use=${port}
	fi
	# Create named pipes for new client comms
	mkfifo ${pipein}
	mkfifo ${pipeout}
	# Make sure we keep the pipes open for writing. (We open for reading
	# and writing because opening in one direction blocks on named pipes.)
	exec 44<>${pipein}
	exec 45<>${pipeout}
	# Start up netcat as our dummy client - anything received over the 
	# input pipe will be sent to the server. 
	if [ "$debug" ] ; then
	    echo "Starting nc as client ${client}" >&2
	fi
	nc -4 localhost ${port_to_use} < ${pipein} > ${pipeout} 2>/dev/null &
	nc_pids[${client}]="$!"
	# Create an empty client output file
	rm -f /tmp/client.$$.${client}.stdout
	touch /tmp/client.$$.${client}.stdout
	clients+=("${client}")
	# netcat will have inherited fds 44 and 45 so we can close them here
	exec 44>&- 45>&-
    fi
    case "${request}" in
	close )
	    # Copy everything remaining from output pipe to stdout file for client
	    if [ "$debug" ] ; then
		echo "Saving everything from client's output and quitting client" >&2
	    fi
	    cat ${pipeout} >> /tmp/client.$$.${client}.stdout &
	    catpid=$!
	    # Need a delay here to (hopefully) start cat reading before we kill the
	    # client (which kills the other end of the pipe)
	    sleep ${delay}
	    # Kill off the client
	    kill -9 ${nc_pids[${client}]} >&/dev/null
	    wait ${nc_pids[${client}]} >&/dev/null
	    wait $catpid >&/dev/null
	    rm -f ${pipein} ${pipeout}
	    unset nc_pids[${client}]
	    continue
	    ;;
	read )
	    unset clientline
	    if read -t 10 -r clientline < ${pipeout} ; then
		# (NOTE: We don't set IFS - leading spaces on the line will be discarded.)
		# Save the line to the client's output file
		if [ "$debug" ] ; then
		    echo "Client ${client} received line '${clientline}'" >&2
		fi
		echo "${clientline}" >> /tmp/client.$$.${client}.stdout
	    else
		# Got timeout which was not expected
		echo "Got unexpected timeout waiting for line from server" >> /tmp/client.$$.${client}.stdout
		if [ "$debug" ] ; then
		    echo "Client ${client} got unexpected timeout reading from server" >&2
		fi
	    fi
	    continue
	    ;;
	readtimeout )
	    unset clientline
	    # (NOTE: We don't set IFS - leading spaces on the line will be discarded.)
	    # We open pipe for reading and writing here so there is no block
	    if read -t 0.5 -r clientline <> ${pipeout} ; then
		# we expected no data but got some
		if [ "$debug" ] ; then
		    echo "Expected timeout but client ${client} got line '${clientline}' from server" >&2
		fi
		# Save the line to the client's output file
		echo "Expected timeout on read, but got unexpected: ${clientline}" >> /tmp/client.$$.${client}.stdout
	    else
		echo "Got expected timeout waiting for line from server" >> /tmp/client.$$.${client}.stdout
		if [ "$debug" ] ; then
		    echo "Got expected timeout for client ${client}" >&2
		fi
	    fi
	    continue
	    ;;
	send )
	    if [ "$debug" ] ; then
		echo "Sending '${mesg}' to ${pipein}" >&2
	    fi
	    echo -e "${mesg}" > ${pipein}
	    ;;
	sendnonewline )
	    if [ "$debug" ] ; then
		echo "Sending '${mesg}' (no newline) to ${pipein}" >&2
	    fi
	    echo -n -e "${mesg}" > ${pipein}
	    ;;
	*) # Other cases (e.g. open, openhttp) have been dealt with
	    ;;
    esac
    sleep "${delay}"
done

# Allow the clients to finish (necessary if $delay is small
sleep 1

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
for client in ${clients[@]} ; do
    echo "Output from client $client is:"
    echo "------------------------"
    cat /tmp/client.$$.${client}.stdout
    echo "------------------------"
done

# Send crackserver's stderr to stderr (excluding the line with the 
# port number on it)
grep -v ${port} /tmp/server.err.$$ >&2

# Remove any temporary files
rm -f /tmp/client.$$.* /tmp/server.out.$$ /tmp/server.err.$$ /tmp/listen.*
exit 0
