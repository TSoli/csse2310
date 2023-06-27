#!/bin/bash
# Usage: run_testuqwordiply_with_interrupt.sh delay args-to-testuqwordiply

if [ $# -lt 3 ]; then
    echo "Expected at least 3 arguments to $0"
    exit 1
fi

delay="$1"
shift

eval /usr/bin/timeout --preserve-status -s INT "$delay" ${testuqwordiply:=./testuqwordiply} ${*@Q}
