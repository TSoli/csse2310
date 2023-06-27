#!/bin/sh

/local/courses/csse2310/bin/demo-uqwordiply "$@"
status=$?
echo "Extra stdout"
exit $status
