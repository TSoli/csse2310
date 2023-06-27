#!/bin/bash
while [ "$1" -a "$2" ] ; do
    case "$1" in
	-3)
	    exec 3<"$2";;
	-4)
	    exec 4<"$2";;
	-3out)
	    exec 3>"$2";;
	-4out)
	    exec 4>"$2";;
	*)
	    echo "Unknown argument to $0" >&2
	    exit 1
    esac
    shift 2
done
eval ${uqcmp:=./uqcmp}
