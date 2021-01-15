#!/bin/sh -l

#!/bin/sh -l

branch="$1"
# shellcheck disable=SC2034 # used by appoptics-bindings in tests
export AO_TOKEN_PROD="$2"
node_version="$3"
# shellcheck disable=SC2034
os_string="$4"
# aws_access_key_id=$5
# aws_secret_access_key=$6

echo "::set-output name=all-args-in-script::$*"

# make os-release one line and get rid of the garbage that confuses github actions
details=$(tr -d '()' < /etc/os-release | tr '\n' ',' | sed 's/ANSI_COLOR="0;3.",//')
echo "::set-output name=os-details::$details"

echo "::set-output name=branch-to-test::$branch with node $node_version"

echo "::set-output name=publish-name::bruce-ness"
echo "::set-output name=publish-destination::somewhere"
