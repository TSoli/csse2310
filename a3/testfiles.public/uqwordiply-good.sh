#!/bin/sh
# uqwordiply-good.sh
# 
# A fully functional version of uqwordiply that allows some different
# behaviour if appropriate environment variables are set
PATH=${PATH}:/bin:/usr/bin
(echo "$(date +%H:%M:%S.%N) Starting $0" >&999) 2>/dev/null

# Set up environment variable so that demo-uqwordiply (run as our program
# under test) logs arguments if required
unset DEMO_UQWORDIPLY_LOG
if [ "${PROGRAM_UNDER_TEST_LOG}" ] ; then
    export DEMO_UQWORDIPLY_LOG=${PROGRAM_UNDER_TEST_LOG}
fi

# Delay if required
if [ "${PROGRAM_UNDER_TEST_DELAY}" ] ; then
    sleep ${PROGRAM_UNDER_TEST_DELAY}
fi

# Run demo-uqwordiply as our program under test
/local/courses/csse2310/bin/demo-uqwordiply "$@"
status=$?
(echo "$(date +%H:%M:%S.%N) Finishing $0" >&999) 2>/dev/null
exit $status
