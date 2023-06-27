#!/bin/sh

/local/courses/csse2310/bin/demo-uqwordiply "$@"
status=$?
echo "Extra stderr" >&2
exit $status
