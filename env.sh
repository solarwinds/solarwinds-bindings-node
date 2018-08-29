#!/bin/sh

ARG=$1
PARAM=$2
PARAM2=$3


#a="/$0"; a=${a%/*}; a=${a#/}; a=${a:-.}; BASEDIR=$(cd "$a"; pwd)
#echo $BASEDIR

# issue informational message if not installing oboe. installing
# oboe only occurs on `npm publish`, `npm pack`, and `npm install`
# (with no arguments, i.e., not on `npm install appoptics-bindings`)
# this section is used for primarily for development and testing.
if [ -z "$AO_TOKEN_STG" -a "$ARG" != "install-new-oboe" ]; then
    echo "AO_TOKEN_STG must be defined and contain a valid token"
    echo "for accessing the appoptics collector specified by"
    echo "AO_TEST_COLLECTOR"
fi

Which=$(which ${0#-})
if [ "$Which" != "/bin/sh" -a "$Which" != "/bin/bash" ]; then
    if [ "$ARG" != "fetch-oboe-version" -a "$ARG" != "install-oboe-version" ]; then
        echo "this script must be sourced so the environment variables can be set"
        echo "only 'fetch-oboe-version' and 'install-oboe-version' work correctly"
        echo "when the script is not sourced."
        /bin/false
        return
    fi
fi

# define this in all cases.
export APPOPTICS_SERVICE_KEY=${AO_TOKEN_STG}:node-bindings-test

#
# this function references the implicit parameters $PARAM and $PARAM2
#
get_new_oboe() {
    # N.B. if installing a new version of oboe "npm run preinstall" must be
    # run before building in order to set up symlinks.
    if [ -z "$PARAM" ]; then
        echo "Must supply a version (which will be used as the destination"
        echo "directory). N.B. the script is not bulletproof."
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
    PAIRS="liboboe-1.0-x86_64.so.0.0.0  liboboe-1.0-alpine-x86_64.so.0.0.0"
    # add the static files if using them.
    #PAIRS="$PAIRS  liboboe-static-alpine-x86_64.gz  liboboe-static-x86_64.gz"
    ERRORS=0
    ERRORFILES=
    #AO_DOWNLOAD_SOURCE="https://s3-us-west-2.amazonaws.com/rc-files-t2/c-lib"
    URL="https://files.appoptics.com/c-lib"
    URL=${AO_DOWNLOAD_SOURCE:-$URL}
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
            echo pretending to download $URL$f to ./oboe-$PARAM/${f}
        else
            echo downloading $URL$f to ./oboe-$PARAM/${f}}
            $downloader "./oboe-$PARAM/${f}" "${URL}$f"
            $downloader "./oboe-$PARAM/${f}.sha256" "${URL}$f.sha256"
        fi

        # check each sha256
        correct=`cat "./oboe-$PARAM/$f.sha256" | awk '{print $1}'`
        checked=`sha256sum "./oboe-$PARAM/$f" | awk '{print $1}'`
        if [ "$checked" != "$correct" -o "$checked" = "" -o "$correct" = "" ]; then
            ERRORS=`expr $ERRORS + 1`
            ERRORFILES="$ERRORFILES $f"
            echo "WARNING: SHA256 for $f DOES NOT MATCH!"
            echo "found    ${checked:-nothing}"
            echo "expected ${correct:-SHA}"
        else
            echo "SHA256 matches expected value for $f"
        fi
    done

    if [ $ERRORS -gt 0 ]; then
        echo "$ERRORS SHA mismatches:$ERRORFILES"
        return 1
    fi

    #
    # the libraries are OK, now get the supporting files. for some reason
    # the directories they are in are not where the source code expects
    # them to be, so fix the directories up by making the 'include'
    # directory go away.
    #
    for f in VERSION \
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
        fi
    done

    # expand the zip files if they were downloaded
    for f in ./oboe-$PARAM/*.gz
    do
        # make this work with/without the static library .gz files
        [ -e "$f" ] || break
        # consider adding '.a' extension?
        gunzip $f
    done

    return 0
}

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
elif [ "$ARG" = "fetch-oboe-version" ]; then
    # this version uses the function
    get_new_oboe

elif [ "$ARG" = "install-oboe-version" ]; then
    # this downloads the new oboe AND moves it to
    # the oboe directory, elevating it to production.
    get_new_oboe

    if [ $? -ne "0" ]; then
        echo "failed to download files"
        /bin/false
        return
    fi

    # get rid of the existing directory
    rm -rf oboe

    # move the new one over
    mv oboe-$PARAM oboe

    echo "a new version of oboe ($PARAM) has been placed in the oboe directory"
    echo "'node setup-liboboe' must be run before building in order to set up"
    echo "the necessary symlinks. 'npm run install' will run 'node setup-liboboe'"
    echo "before building, so the separate step isn't necessary."

else
    echo "ERROR $ARG invalid"
fi



return
