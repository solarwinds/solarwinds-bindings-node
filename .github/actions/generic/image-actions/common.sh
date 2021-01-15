#!/bin/sh -l

#
# this file looks at the first argument to determine whether to run the
# build-and-test-bindings.sh script or the publish-bindings.sh script.
#
script_to_run="/image-scripts/$1"

if [ ! -f "$script_to_run" ] || [ ! -x "$script_to_run" ]; then
    if [ "$script_to_run" != "echo" ]; then
        echo "not an executable file: ${script_to_run}"
        exit 1
    fi
fi

if [ ! $# = 5 ]; then
    echo "expected exactly 5 arguments"
    exit 1
fi

"$script_to_run" "$2" "$3" "$4" "$5"
