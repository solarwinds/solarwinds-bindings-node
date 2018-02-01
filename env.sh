ARG=$1

if [[ -z "$AO_TOKEN_STG" ]]; then
    echo "AO_TOKEN_STG must be defined and contain a valid token"
    echo "for accessing collector-stg.appoptics.com"
    return
fi

# define this for all consitions
export APPOPTICS_SERVICE_KEY=${AO_TOKEN_STG}:node-bindings-text

if [[ -z "$ARG" ]]; then
    echo "source this script with an argument of udp or ssl. it"
    echo "will define environment variables to enable testing with"
    echo "the specified reporter".
    echo
    echo "you may also use the argument debug to define additional"
    echo "debugging variables"
    echo
elif [[ "$ARG" = "udp" ]]; then
    export APPOPTICS_REPORTER=udp
    export APPOPTICS_REPORTER_UDP=localhost:7832
elif [[ "$ARG" = "ssl" ]]; then
    export APPOPTICS_REPORTER=ssl
    export APPOPTICS_COLLECTOR=collector-stg.appoptics.com
elif [[ "$ARG" = "debug" ]]; then
    export APPOPTICS_DEBUG_LEVEL=6
    export APPOPTICS_SHOW_GYP=1
else
    echo "ERROR $ARG invalid"
fi

return












