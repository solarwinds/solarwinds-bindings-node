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
# temporary fix for gha not supporting arm64 alpine < 3.17
JSON_WITH_ARCH=$(echo "$JSON_WITH_ARCH" | jq -c '.include = [.include[] | select((.arch == "arm64") and (.image | contains("alpine")) and (.image | contains("alpine3.17") | not) | not)]')

ARM64_IMAGES=$(echo "$JSON_WITH_ARCH" | jq -r '.include[] | select(.arch == "arm64") | .image')

# return a value
echo "matrix=$JSON" >> $GITHUB_OUTPUT
echo "matrix-with-arch=$JSON_WITH_ARCH" >> $GITHUB_OUTPUT
echo "arm64-images<<EOF" >> $GITHUB_OUTPUT
echo "$ARM64_IMAGES" >> $GITHUB_OUTPUT
echo "EOF" >> $GITHUB_OUTPUT
