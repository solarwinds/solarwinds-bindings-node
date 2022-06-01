#!/bin/bash

# input is a space delimited string of files to checksum
# e.g. "liboboe-1.0-x86_64.so.0.0.0 liboboe-1.0-alpine-x86_64.so.0.0.0 liboboe-1.0-lambda-x86_64.so.0.0.0"
# we assume each file has a matching .sha256 file.
files="$1"

# hard coded location of sha files
url="https://agent-binaries.cloud.solarwinds.com/apm/c-lib"

errors=0
error_files=""

for f in $files; do
    # get data from binary file and sha256 file
    version=$(cat ./oboe/VERSION)
    echo "Downloading ${url}/${version}/$f.sha256"
    curl -o "./oboe/${f}.sha256" "${url}/${version}/$f.sha256"

    correct=$(awk '{print $1}' < "./oboe/$f.sha256")
    checked=$(sha256sum "./oboe/$f" | awk '{print $1}')
    
    # All cases
    if [ "$checked" != "$correct" ] || [ "$checked" = "" ] || [ "$correct" = "" ]; then
        errors=$((errors + 1))
        error_files="$error_files $f"
        echo "WARNING: SHA256 for $f DOES NOT MATCH!"
        echo "found    ${checked:-nothing}"
        echo "expected ${correct:-SHA}"
    else
        echo "SHA256 matches expected value for $f"
    fi
done 

if [ "$errors" -gt 0 ]; then
    echo "$errors SHA mismatches:$error_files"
    exit 1 # Exit with error will halt workflow run
fi
