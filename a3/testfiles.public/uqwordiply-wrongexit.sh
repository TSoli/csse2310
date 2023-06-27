#!/bin/sh

/local/courses/csse2310/bin/demo-uqwordiply "$@"
status=$?
# Exit with a status one greater than we were supposed to
exit $((++status))
