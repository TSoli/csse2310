#!/bin/bash
# check_child_pipes.sh sample-time pattern args-to-testuqwordiply
# Prints details of a pipe in the children that matches the given pattern at
# the given sample time after commencement of the test
args="${*@Q}"
(echo -e "\n$(date +%H:%M:%S.%N) Starting $0 ${args}" >&999) 2>/dev/null
export PATH=${PATH}:/local/courses/csse2310/bin:/bin:/usr/bin
if [ $# -lt 4 ] ; then
    echo "Invalid arguments"
    exit 1
fi
delay="$1"
pattern="$2"
shift 2

# Run testuqwordiply with given arguments - ensure our test programs do
# nothing for the first 1.6 seconds so we can sample their fds
export DEMO_UQWORDIPLY_DELAY=1.6 
export UQCMP_DELAY=1.6
${testuqwordiply:=./testuqwordiply} "$@" &
parentpid=$!

# Allow enough time for all the child processes to be alive
/usr/bin/sleep ${delay}

# Get child pids of this process (and grand children)
children=$(/usr/bin/pgrep -d " " -P ${parentpid})
numchildren=$(wc -w <<< "$children")
if [ $numchildren = 0 ] ; then
    # No child processes found
    echo "No children of testuqwordiply found" > /tmp/pipe.out
    exit 1
elif [ $numchildren = 1 ] ; then
    grandchildren=$(for i in $children ; do /usr/bin/pgrep -d " " -P $i; done;)
   children=$(echo $grandchildren)
fi

(echo " $(date +%H:%M:%S.%N) Found child processes: $children" >&999) 2>/dev/null

# Get details about the file descriptors 0 to 4 for these processes
# Args are:
#	-a	AND following constraints
#	-p	process IDs of interest (comma separated)
#	-d	file descriptors of interest
#	-E	Show details of the other end of pipes in the name field
#	-O	Get lsof to try and avoid kernel blocks
#	-F	format of output. Fields will be p (pid), c (comamnd name),
#		f (fd), a (access mode), t (type, e.g. FIFO), n (file name
#		or pipe details)
/usr/bin/lsof -a -p ${children// /,} -d 0-4 -E -O -F atcn > /tmp/lsof.$$.out
(echo "$(date +%H:%M:%S.%N) lsof output:" >&999) 2>/dev/null
(cat /tmp/lsof.$$.out >&999) 2>/dev/null
cat /tmp/lsof.$$.out |
/usr/bin/gawk 'BEGIN {FS = "[, ]"; row=0;}
/^p/ { pid=substr($0,2); }
/^c/ { cmd=substr($0,2); cmdNames[pid]=cmd; }
/^f/ { fd=substr($0,2); }
/^a/ { mode=substr($0,2); }
/^npipe/ { for(i=2; i<NF; i+= 3) {
		j=i+2;
		data[row++] = sprintf("%s %d%c %s %s", pid, fd, mode, $i, $j);
	    }
	}
END { for(i=0; i< row; i++) {
	for(pid in cmdNames) {
	    data[i]=gensub(pid,cmdNames[pid], "g", data[i]);
	}
    }
    for(i=0; i < row; i++) {
	if(!match(data[i], " [0-9]+ ")) {
	    print data[i];
	}
    }
}
' | /usr/bin/sort | /usr/bin/grep -e "${pattern}" > /tmp/pipe.out

wait ${parentpid} >&/dev/null
status=$?
(echo "$(date +%H:%M:%S.%N) Finishing $0 ${args} - /tmp/pipe.out contains:----" >&999
 cat /tmp/pipe.out >&999; echo "----">&999) 2>/dev/null
rm /tmp/lsof.*.out
exit $status
