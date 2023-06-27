#!/bin/bash
# Usage: run_wordle.sh answer guess-file [-lines n] [args]
# Runs wordle with the given word as the answer with stdin redirected from
# the given guess file (one guess per line). 
# If the -lines argument is given, then only the given number of lines
# from standard output will be saved - others will be discarded.
# Additional args are given as command line arguments to wordle.
if [ "$#" -lt 2 ] ; then
    echo "Insufficient arguments to $0" >&2
    exit 1
fi
if [ ! -r "$2" ] ; then
    echo "Can't read guesses file \"$2\"" >&2
    exit 2
fi
answer="$1"
guesses="$2"
shift 2
if [ "$1" = "-lines" -a -n "$2" ] ; then
    lines="$2"
    shift 2;
else
    lines=-0
fi
export WORD2310="${answer}"
${wordle:=./wordle} "$@" < ${guesses} > /tmp/$$.out
status=$?
head -n "$lines" < /tmp/$$.out
rm -f /tmp/$$.out
exit $status
