#!/bin/sh

ARG=$1
PARAM=$2
PARAM2=$3

# where to get files: STAGING or PRODUCTION (default)
SOURCE=${SOURCE:-PRODUCTION}

#a="/$0"; a=${a%/*}; a=${a#/}; a=${a:-.}; BASEDIR=$(cd "$a"; pwd)
#echo $BASEDIR

Which=$(which "${0#-}")

case $Which in
    */bin/sh)
    ;;
    */bin/bash)
    ;;

    *)
        # it's kind of kludgy but all args that involve oboe contain the string
        # "oboe-version". so if the command doesn't contain oboe-version it must
        # be sourced
        if test "${ARG#*oboe-version}" = "$ARG" ; then
            echo "this script must be sourced so the environment variables can be set."
            echo "only the '*-oboe-version' commands work correctly when the script is"
            echo "executed and not sourced."
            /bin/false
            return
        fi
    ;;
esac

# define this in all cases.
if [ "$ARG" = "stg" ]; then
    [ -z "$AO_TOKEN_STG" ] && echo "Missing AO_TOKEN_STG, aborting" && return 1
    export APPOPTICS_SERVICE_KEY=${AO_TOKEN_STG}:node-bindings-test
elif [ "$ARG" = "prod" ]; then
    [ -z "$AO_TOKEN_PROD" ] && echo "Missing AO_TOKEN_PROD, aborting" && return 1
    export APPOPTICS_SERVICE_KEY=${AO_TOKEN_PROD}:node-bindings-test
fi
#
# this function references the implicit parameters $PARAM and $PARAM2
#
get_new_oboe() {
    # N.B. if installing a new version of oboe setup-liboboe.js must be
    # run in order to set up symlinks.
    if [ -z "$PARAM" ]; then
        echo "Must supply a version (which will be used as the destination"
        echo "directory). N.B. the script is not bulletproof."
        echo "example:"
        echo "$ . env.sh fetch-oboe-version latest"
        echo "or:"
        echo "$ . env.sh install-oboe-version 4.1.0"
        echo "or if wanting to fetch from staging rather than production:"
        echo "$ SOURCE=STAGING . env.sh install-oboe-version 6.0.0"
        return
    fi
    if [ "$(which wget)" ]; then
        downloader="wget -q -O"
    elif [ "$(which curl)" ]; then
        downloader="curl -o"
    else
        echo "Neither wget nor curl is available, aborting"
        return 1
    fi
    # pretend to download for testing by adding any extra parameter
    PRETEND=$PARAM2
    PAIRS="liboboe-1.0-x86_64.so.0.0.0  liboboe-1.0-alpine-x86_64.so.0.0.0 liboboe-1.0-lambda-x86_64.so.0.0.0"
    # add the libressl version if we are using it again.
    # PAIRS="$PAIRS liboboe-1.0-alpine-libressl-x86_64.so.0.0.0"
    # a short window of oboe versions don't have multiple versions for alpine.
    # OKMISSING needs a leading blank due to the concatenation to ERRORFILES.
    OKMISSING=" liboboe-1.0-alpine-libressl-x86_64.so.0.0.0"
    # add the static files if using them.
    #PAIRS="$PAIRS liboboe-static-alpine-x86_64.gz liboboe-static-x86_64.gz"
    ERRORS=0
    ERRORFILES=

    # presume production
    URL="https://files.appoptics.com/c-lib"
    if [ "$SOURCE" = "STAGING" ]; then
        URL="https://s3-us-west-2.amazonaws.com/rc-files-t2/c-lib"
    elif [ "$SOURCE" != "PRODUCTION" ]; then
        echo "Invalid SOURCE value $SOURCE, aborting"
        exit 1
    fi
    URL="$URL/$PARAM/"

    # create the primary directory for this version of oboe
    mkdir -p "./oboe-$PARAM"

    #
    # get the libraries and their SHA256 hashes. they all go in
    # the primary directory.
    #
    for f in $PAIRS
    do
        if [ -n "$PRETEND" ]; then
            echo pretending to download "$URL$f" to "./oboe-$PARAM/${f}"
        else
            echo downloading "$URL$f" to "./oboe-$PARAM/${f}"
            $downloader "./oboe-$PARAM/${f}" "${URL}$f"
            $downloader "./oboe-$PARAM/${f}.sha256" "${URL}$f.sha256"
        fi

        # check each sha256
        correct=$(awk '{print $1}' < "./oboe-$PARAM/$f.sha256")
        checked=$(sha256sum "./oboe-$PARAM/$f" | awk '{print $1}')
        if [ "$checked" != "$correct" ] || [ "$checked" = "" ] || [ "$correct" = "" ]; then
            if [ "$PRETEND" = "" ]; then
              ERRORS=$((ERRORS + 1))
              ERRORFILES="$ERRORFILES $f"
              echo "WARNING: SHA256 for $f DOES NOT MATCH!"
              echo "found    ${checked:-nothing}"
              echo "expected ${correct:-SHA}"
            fi
        else
            echo "SHA256 matches expected value for $f"
        fi
    done

    if [ "$ERRORS" -gt 0 ] && [ "$ERRORFILES" != "$OKMISSING" ]; then
        echo "$ERRORS SHA mismatches:$ERRORFILES"
        return 1
    fi

    #
    # the libraries are OK, now get the supporting files. for some reason
    # the directories they are in are not where the source code expects
    # them to be, so fix the directories up by making the 'include'
    # directory go away.
    #
    for f in VERSION ChangeLog \
        include/oboe.h include/oboe_debug.h \
        include/bson/bson.h include/bson/platform_hacks.h
    do
        if [ -n "$PRETEND" ]; then
            echo pretending to download $f to "./oboe-$PARAM/${f#include/}"
        else
            echo downloading $f as "./oboe-$PARAM/${f#include/}"
            # wget has no --create-dirs, so brute force directory creation
            mkdir -p "$(dirname "./oboe-$PARAM/${f#include/}")"
            $downloader "./oboe-$PARAM/${f#include/}" "${URL}$f"
        fi
    done

    # expand the zip files if they were downloaded
    for f in "./oboe-$PARAM/"*.gz
    do
        # make this work with/without the static library .gz files
        [ -e "$f" ] || break
        # consider adding '.a' extension?
        gunzip "$f"
    done

    return 0
}

if [ -z "$ARG" ]; then
    echo "source this script with an argument of udp or stg or prod. it"
    echo "will define environment variables to enable testing with"
    echo "the specified reporter".
    echo
    echo "you may also use the argument debug to define additional"
    echo "debugging variables"
    echo
elif [ "$ARG" = "udp" ]; then
    export APPOPTICS_REPORTER=udp
    export APPOPTICS_COLLECTOR=${AO_TEST_COLLECTOR:-localhost:7832}
elif [ "$ARG" = "stg" ]; then
    export APPOPTICS_REPORTER=ssl
    export APPOPTICS_COLLECTOR=${AO_TEST_COLLECTOR:-collector-stg.appoptics.com}
elif [ "$ARG" = "prod" ]; then
    export APPOPTICS_REPORTER=ssl
    export APPOPTICS_COLLECTOR=${AO_TEST_COLLECTOR:-collector.appoptics.com}
elif [ "$ARG" = "debug" ]; then
    export APPOPTICS_DEBUG_LEVEL=6
    export APPOPTICS_SHOW_GYP=1
elif [ "$ARG" = "fetch-oboe-version" ]; then
    # this version uses the function
    get_new_oboe

elif [ "$ARG" = "install-local-oboe-version" ]; then
    # promote a downloaded version to the production
    # directory 'oboe'
    if [ ! -d "oboe-$PARAM" ]; then
        echo "Can't find directory ./oboe-$PARAM"
    else
        # counts on oboe-$PARAM having the correct files
        echo "rm -rf oboe"
        rm -rf oboe
        echo "cp -r oboe-$PARAM oboe"
        cp -r "oboe-$PARAM" oboe
        echo "[done]"
    fi

elif [ "$ARG" = "install-oboe-version" ]; then
    # this downloads the new oboe AND moves it to
    # the oboe directory, elevating it to production.
    if ! get_new_oboe; then
        echo "failed to download files"
        /bin/false
        return
    fi

    # get rid of the existing directory
    rm -rf oboe

    # move the new one over
    mv "oboe-$PARAM" oboe

    [ -n "$PRETEND" ] && echo "PRETEND THAT"
    echo "a new version of oboe ($PARAM) has been placed in the oboe directory"
    echo "'node setup-liboboe' must be run before building in order to set up"
    echo "the necessary symlinks. 'npm run install' will run 'node setup-liboboe'"
    echo "before building, so the separate step isn't necessary."

else
    echo "ERROR $ARG invalid"
fi



return
