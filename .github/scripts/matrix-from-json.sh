#!/bin/bash

path_to_file="$1"

# escape human readable multi line json
# to format usable by github fromJSON.
# see https://github.com/actions/toolkit/issues/403
# and https://github.com/actions/toolkit/blob/master/packages/core/src/command.ts#L92
JSON=$(cat "$path_to_file")
JSON="${JSON//'%'/'%25'}"
JSON="${JSON//$'\n'/'%0A'}"
JSON="${JSON//$'\r'/'%0D'}"

# return a value
echo "::set-output name=matrix::$JSON"
