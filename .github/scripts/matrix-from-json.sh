#!/bin/bash

path_to_file="$1"

# escape human readable multi line json
# to format usable by github fromJSON.
# see https://github.com/actions/toolkit/issues/403
# and https://github.com/actions/toolkit/blob/master/packages/core/src/command.ts#L92
JSON=$(cat "$path_to_file")
JSON="${JSON//$'\n'/''}"
JSON="${JSON//$'\r'/''}"
# enable the JSON to use any environment var
# replace the = sign with a space and turn into array
for env_var in $(printenv); do
  read -ra arr <<< "${env_var//=/ }"
  JSON="${JSON//\$${arr[0]}/"${arr[1]}"}"
done

JSON_WITH_ARCH=$(echo "$JSON" | jq -c '.include = [.include[] | .+{ arch: ["arm64", "x64"][] }]')
IMAGES=$(echo "$JSON" | jq -r '.include[].image')

# return a value
echo "matrix=$JSON" >> $GITHUB_OUTPUT
echo "matrix-with-arch=$JSON_WITH_ARCH" >> $GITHUB_OUTPUT
echo "images<<EOF" >> $GITHUB_OUTPUT
echo "$IMAGES" >> $GITHUB_OUTPUT
echo "EOF" >> $GITHUB_OUTPUT
