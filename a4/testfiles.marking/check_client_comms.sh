#!/bin/bash
# Usage: check_client_comms.sh server.response.file [client.job.file]
# Report messages received by the server, and messages received by the client
# The client's stdin from the file given on the command line or from this 
# scripts stdin. The server's stdin (response) comes from the file given 
# on the command line

if [ $# -lt 1 ] ; then
    echo "Incorrect number of arguments" >&2
    exit 1
fi
server_reply="$1"
if [ "$2" ] ; then
    client_job_file="$2"
fi

rm -f /tmp/server.in /tmp/client.out

# Determine a free port number for our dummy server to listen on
port=$(testfiles/freeport.sh)

# Start up netcat as a dummy server in the background - timing out after 5 secs
echo "Text received by server is:" > /tmp/server.in
#/local/courses/csse2310/bin/slowcat.sh -t 0.4 ${server_reply} | timeout 5 nc --no-shutdown -v -l -4 ${port} >> /tmp/server.in 2>/dev/null &
/local/courses/csse2310/bin/slowcat.sh -t 0.4 ${server_reply} | timeout 5 nc -v -l -4 ${port} >> /tmp/server.in 2>/dev/null &
nc_pid=$!

# Wait half a second to ensure our dummy server has started.
sleep 0.5

# Run crackclient with our arguments - timeout after 3 seconds
echo "Text output by crackclient is:" >/tmp/client.out
if [ "${client_job_file}" ] ; then
    timeout 3 ${crackclient:=./crackclient} ${port} ${client_job_file} >> /tmp/client.out
    status=$?
else
    timeout 3 ${crackclient:=./crackclient} ${port} >> /tmp/client.out
    status=$?
fi

# Make sure dummy server has finished
kill $nc_pid >&/dev/null
wait $nc_pid >&/dev/null

exit $status
