#!/bin/bash
# check_crackserver_http_port.sh -notset|-good|-bad|-bothbad
# We use a good primary port (unless -bothbad is used)
# and check for listening there before we
# check the HTTP port

rm -f /tmp/stderr

unset A4_HTTP_PORT nc_pid

# Choose a couple of free ports
port=$(testfiles/freeport.sh)
httpport=$(testfiles/freeport.sh)

case "$1" in 
    -notset ) ;;
    -good )
	export A4_HTTP_PORT=${httpport}
	;;
    -bad ) 
	export A4_HTTP_PORT=${httpport}
	# Start a dummy server to occupy the http port
	nc --no-shutdown -4 -l ${httpport} < /dev/null >& /dev/null &
	nc_http_pid=$!
	testfiles/wait_until_listening.sh $nc_http_pid $httpport
	;;
    -bothbad )
	export A4_HTTP_PORT=${httpport}
	# Start a dummy server to occupy the primary port
	nc --no-shutdown -4 -l ${port} </dev/null >& /dev/null &
	nc_pid=$!
	# Start a dummy server to occupy the http port
	nc --no-shutdown -4 -l ${httpport} </dev/null >& /dev/null &
	nc_http_pid=$!
	testfiles/wait_until_listening.sh $nc_pid $port
	testfiles/wait_until_listening.sh $nc_http_pid $httpport
	;;
    * )
	echo "Bad argument: $1" >&2
	exit 1;
esac


# Start up crackserver in the background and wait until listening (or exited)
${crackserver:=./crackserver} --port ${port} 2>/tmp/stderr &
server_pid=$!
testfiles/wait_until_listening.sh $server_pid $port
sleep 0.5

# Check what ports it is listening on
allports=$(testfiles/report_listening_ports.sh $server_pid)
getportstatus=$?

status=0
case "$1" in 
    -notset )
	if [[ ${port} = ${allports} ]] ; then
	    echo "Server listening only on primary port as expected"
	else
	    echo "Server was listening on ports ${allports} instead of only ${port}"
	    status=1
	fi
	;;
    -bad | -bothbad )
	if [ ${getportstatus} -ne 0 ] ; then
	    echo "Server exited as expected"
	else
	    echo "Server was listening on ports ${allports} but should have exited"
	    status=1
	fi
	;;
    -good )
	expected=$(echo $(echo -e "${port}\n${httpport}" | sort -n))
	if [[ ${expected} = ${allports} ]] ; then
	    echo "Server listening only on both ports as expected"
	else
	    echo "Server was listening on ports ${allports} instead of ${expected}"
	    status=1
	fi
	;;
esac

sleep 0.5

# Kill off the server under test
kill -TERM $server_pid >&/dev/null || kill -KILL $server_pid >&/dev/null
wait $server_pid >&/dev/null

# Kill off the dummy server(s) (if any)
if [ "$nc_pid" ] ; then
    kill -9 $nc_pid >&/dev/null
    wait $nc_pid >&/dev/null
fi
if [ "$nc_http_pid" ] ; then
    kill -9 $nc_http_pid >&/dev/null
    wait $nc_http_pid >&/dev/null
fi

# Output crackserver's standard error to standard error here (minus any line 
# containing the port number)
grep -v ${port} /tmp/stderr >&2
rm -f /tmp/stderr*

exit $status
