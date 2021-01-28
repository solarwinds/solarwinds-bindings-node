#!/bin/sh -l

#
# this file looks at the first argument to determine whether to run the
# node-os-tests.sh script or the publish.sh script.
#
script_name=$1
script_to_run="/image-scripts/$1"

if [ ! -f "$script_to_run" ] || [ ! -x "$script_to_run" ]; then
    if [ "$script_to_run" != "echo" ]; then
        echo "? error: not an executable file: ${script_to_run}"
        exit 1
    fi
fi

if [ "$script_name" = "node-os-tests.sh" ]; then
    if [ $# -lt 5 ]; then
        echo "? error: node-os-tests.sh requires 5 arguments, not $#"
        echo "args: $*"
        exit 1
    fi
elif [ "$script_name" = "publish.sh" ]; then
    if [ $# -lt 8 ]; then
        echo "? error: publish.sh requires 8 arguments, not $#"
        echo "args: $*"
        exit 1
    fi
else
    echo "unknown script: ${script_name}"
    exit 1
fi

"$script_to_run" "$2" "$3" "$4" "$5" "$6" "$7" "$8"
