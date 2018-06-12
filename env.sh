ARG=$1
PARAM=$2
PARAM2=$3

if [ -z "$AO_TOKEN_STG" ]; then
    echo "AO_TOKEN_STG must be defined and contain a valid token"
    echo "for accessing the appoptics collector specified by"
    echo "AO_TEST_COLLECTOR"
    return
fi

# define this in all cases.
export APPOPTICS_SERVICE_KEY=${AO_TOKEN_STG}:node-bindings-test

if [ -z "$ARG" ]; then
    echo "source this script with an argument of udp or ssl. it"
    echo "will define environment variables to enable testing with"
    echo "the specified reporter".
    echo
    echo "you may also use the argument debug to define additional"
    echo "debugging variables"
    echo
elif [ "$ARG" = "udp" ]; then
    export APPOPTICS_REPORTER=udp
    export APPOPTICS_REPORTER_UDP=localhost:7832
elif [ "$ARG" = "stg" ]; then
    export APPOPTICS_REPORTER=ssl
    export APPOPTICS_COLLECTOR=${AO_TEST_COLLECTOR:-collector-stg.appoptics.com}
elif [ "$ARG" = "prod" ]; then
    export APPOPTICS_REPORTER=ssl
    export APPOPTICS_COLLECTOR=${AO_TEST_COLLECTOR:-collector.appoptics.com}
elif [ "$ARG" = "debug" ]; then
    export APPOPTICS_DEBUG_LEVEL=6
    export APPOPTICS_SHOW_GYP=1
elif [ "$ARG" = "get-new-oboe" ]; then
    # N.B. if installing a new version of oboe "npm run preinstall" must be
    # run before building in order to set up symlinks.
    # can't use https://files.appoptics.com/c-lib (only .so and oboe.h)
    # https://s3-us-west-2.amazonaws.com/rc-files-t2/c-lib/latest/
    # c-lib/latest/liboboe-1.0-x86_64.so.0.0.0
    if [ -z "$PARAM" ]; then
        echo "Must supply a version (which will be used as the destination"
        echo "directory). N.B. this isn't bullet-proof. It presumes sha256sum"
        echo "exists and that you're running bash."
        echo "example:"
        echo "$ . env.sh new-get-new-oboe latest"
        return
    fi
    if [ $(which wget) ]; then
        downloader="wget -q -O"
    elif [ $(which curl) ]; then
        downloader="curl -o"
    else
        echo "Neither wget nor curl is available, aborting"
        return 1
    fi
    # pretend to download for testing by adding an extra parameter
    PRETEND=$PARAM2
    OBOE_X=liboboe-1.0-x86_64.so.0.0.0
    OBOE_A=liboboe-1.0-alpine-x86_64.so.0.0.0
    URL="https://s3-us-west-2.amazonaws.com/rc-files-t2/c-lib/$PARAM/"
    mkdir -p "./oboe-$PARAM"
    for f in VERSION "$OBOE_X" "$OBOE_X.sha256" "$OBOE_A" "$OBOE_A.sha256" \
        include/oboe.h include/oboe_debug.h \
        include/bson/bson.h include/bson/platform_hacks.h
    do
        if [ -n "$PRETEND" ]; then
            echo pretending to download $f to ./oboe-$PARAM/${f#include/}
        else
            echo downloading $f as ./oboe-$PARAM/${f#include/}
            # wget has no --create-dirs, so brute force directory creation
            mkdir -p $(dirname "./oboe-$PARAM/${f#include/}")
            $downloader "./oboe-$PARAM/${f#include/}" "${URL}$f"
            #curl --create-dirs -o "./oboe-$PARAM/${f#include/}" "${URL}$f"
        fi
    done

    # check the sha256's
    correct=`cat "./oboe-$PARAM/$OBOE_X.sha256" | awk '{print $1}'`
    checked=`sha256sum "./oboe-$PARAM/$OBOE_X" | awk '{print $1}'`
    if [ "$checked" != "$correct" -o "$checked" = "" -o "$correct" = "" ]; then
        echo "WARNING: SHA256 for ${OBOE_X} DOES NOT MATCH!"
        echo "found    $checked"
        echo "expected $correct"
    else
        echo "SHA256 matches expected value for $OBOE_X"
    fi

    correct=`cat "./oboe-$PARAM/$OBOE_A.sha256" | awk '{print $1}'`
    checked=`sha256sum "./oboe-$PARAM/$OBOE_A" | awk '{print $1}'`
    if [ "$checked" != "$correct" -o "$checked" = "" -o "$correct" = "" ]; then
        echo "WARNING: SHA256 for ${OBOE_A} DOES NOT MATCH!"
        echo "found    $checked"
        echo "expected $correct"
    else
        echo "SHA256 matches expected value for $OBOE_A"
    fi

else
    echo "ERROR $ARG invalid"
fi

return
