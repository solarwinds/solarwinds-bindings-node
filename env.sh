ARG=$1
PARAM=$2

if [[ -z "$AO_TOKEN_STG" ]]; then
    echo "AO_TOKEN_STG must be defined and contain a valid token"
    echo "for accessing the appoptics collector specified by"
    echo "AO_TEST_COLLECTOR"
    return
fi

# define this in all cases.
export APPOPTICS_SERVICE_KEY=${AO_TOKEN_STG}:node-bindings-test

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
    export APPOPTICS_COLLECTOR=${AO_TEST_COLLECTOR:-collector.appoptics.com}
elif [[ "$ARG" = "debug" ]]; then
    export APPOPTICS_DEBUG_LEVEL=6
    export APPOPTICS_SHOW_GYP=1
elif [[ "$ARG" = "get-new-oboe" ]]; then
    # N.B. if installing a new version of oboe "npm run preinstall" must be
    # run before building in order to set up symlinks.
    # can't really use https://files.appoptics.com/c-lib (only .so and oboe.h)
    # https://s3-us-west-2.amazonaws.com/rc-files-t2/c-lib/latest/
    # c-lib/latest/liboboe-1.0-x86_64.so.0.0.0
    if [[ -z "$PARAM" ]]; then
        echo "Must supply a version (which will be used as the destination"
        echo "directory). N.B. this isn't bullet-proof. It presumes sha256sum"
        echo "exists and that you're running bash."
        echo "example:"
        echo "$ . env.sh get-new-oboe latest"
        return
    fi
    OBOE_NAME=liboboe-1.0-x86_64.so.0.0.0
    URL="https://s3-us-west-2.amazonaws.com/rc-files-t2/c-lib/$PARAM/"
    mkdir -p "./$PARAM"
    for f in oboe.h oboe_debug.h VERSION "$OBOE_NAME" "$OBOE_NAME.sha256"
    do
        #echo "pretending to download $f"
        curl -o "./$PARAM/$f" "${URL}$f"
    done
    # check the sha256
    correct=$(cat "./$PARAM/$OBOE_NAME.sha256")
    read -ra sha256 <<< $(sha256sum "./$PARAM/$OBOE_NAME")
    if [[ "${sha256[0]}" != "$correct" ]]; then
        echo "WARNING: SHA256 DOES NOT MATCH!"
        echo "found    ${sha256[0]} (${#sha256[0]})"
        echo "expected $correct (${#correct})"
    fi


else
    echo "ERROR $ARG invalid"
fi

return
