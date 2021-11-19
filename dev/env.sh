#!/bin/sh

ARG=$1

if [ ! -f /.dockerenv ]; then 
  echo "Must run from inside the dev container."
  exit 
fi

Which=$(which "${0#-}")

case $Which in
    */bin/sh)
    ;;
    */bin/bash)
    ;;

    *)
    echo
    echo "this script must be sourced so the environment variables can be set."
    echo
    /bin/false
    return
    ;;
esac

# define this in all cases.
if [ "$ARG" = "stg" ]; then
    [ -z "$AO_TOKEN_STG" ] && echo "Missing AO_TOKEN_STG, aborting" && return 1
    export APPOPTICS_SERVICE_KEY="${AO_TOKEN_STG}":node-bindings-test
elif [ "$ARG" = "prod" ]; then
    [ -z "$AO_TOKEN_PROD" ] && echo "Missing AO_TOKEN_PROD, aborting" && return 1
    export APPOPTICS_SERVICE_KEY="${AO_TOKEN_PROD}":node-bindings-test
fi


if [ "$ARG" = "udp" ]; then
    export APPOPTICS_REPORTER=udp
    export APPOPTICS_COLLECTOR="${AO_TEST_COLLECTOR:-localhost:7832}"
elif [ "$ARG" = "stg" ]; then
    export APPOPTICS_REPORTER=ssl
    export APPOPTICS_COLLECTOR="${AO_TEST_COLLECTOR:-collector-stg.appoptics.com}"
elif [ "$ARG" = "prod" ]; then
    export APPOPTICS_REPORTER=ssl
    export APPOPTICS_COLLECTOR="${AO_TEST_COLLECTOR:-collector.appoptics.com}"
else
    echo
    echo "source this script with an argument of udp or stg or prod. it"
    echo "will define environment variables to enable testing with"
    echo "the specified reporter".
    echo
fi

return
