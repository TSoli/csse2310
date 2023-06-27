#!/bin/bash
# check_client_can_connect.sh [-service]
# Checks whether crackclient can connect to a server. We use netcat (nc) as a
# dummy server. If "-service" is given on the command line then we 
# we attempt to connect using a service name rather than port number.

rm -f /tmp/$$.out

# Get a free port to listen on
if [ "$1" = "-service" ] ; then
    service=($(testfiles/freeservice.sh))
    name=${service[1]}
    port=${service[0]}
else 
    port=$(testfiles/freeport.sh)
    name=$port
fi
# Run netcat as a dummy server listening on that port. We use verbose mode
# so it will report any connections
nc -v -l -4 ${port} < /dev/null 1>/dev/null 2>/tmp/$$.out  &
netcat_pid=$!

# Make sure nc is listening
if ! testfiles/wait_until_listening.sh ${netcat_pid} ${port} ; then
    echo "Dummy server failed to listen - aborting" >&2
fi

# Run the client for two seconds - should connect in this time.
timeout 2 ${crackclient:=./crackclient} ${name} /etc/resolv.conf >& /dev/null

# The "server" (netcat) should have died when the client disconnected, 
# but kill to be sure
kill $netcat_pid &>/dev/null
wait $netcat_pid &>/dev/null
status=$?

# Check whether the server reported a connection from the client or not 
if grep "Connection from" /tmp/$$.out >&/dev/null ; then
    echo Got connection >&2
    result=0
else
    echo Server did not report connection >&2
    result=1
fi
rm -f /tmp/$$.out
exit $result
