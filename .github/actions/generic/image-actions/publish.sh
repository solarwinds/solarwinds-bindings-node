#!/bin/sh -lx

branch="$1"
# for publish this is user:repo-access-token, for testing AO_TOKEN_PROD. this is
# because apm-node-bindings-actions-test is a private repo that i set up to facilitate
# testing; workflow files must be present in master to be invoked and having to merge
# to master makes testing more difficult.
REPO_ACCESS_URL="$2"
node_version="$3"
# shellcheck disable=SC2034
os_string="$4"
export AWS_ACCESS_KEY_ID="$5"           # provides write access to target bucket
export AWS_SECRET_ACCESS_KEY="$6"
production="$7"

if [ "$production" = "true" ]; then
    s3_host="production"
else
    s3_host="staging"
fi
echo "[set s3_host: $s3_host]"


echo "::set-output name=all-args-in-script::$*"

# make os-release one line and get rid of the garbage that confuses github actions
details=$(tr -d '()' < /etc/os-release | tr '\n' ',' | sed 's/ANSI_COLOR="0;3.",//')
echo "::set-output name=os-details::$details"

# if node isn't installed them nvm should be (handled by <os>.Dockerfile)
if ! which node; then
    [ -d "$HOME/.nvm" ] || exit 1
    export NVM_DIR="$HOME/.nvm"
    # shellcheck disable=SC1090
    [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"
    # no progress saves 100+ lines of log output
    nvm install --no-progress "$node_version"
fi

# get to the actions-provided working directory.
cd "$GITHUB_WORKSPACE" || exit 1
git clone --depth=1 "${REPO_ACCESS_URL}" aob -b "$branch"
cd aob || exit 1

# npm issues the wonderful message "cannot run in wd" when run as root without --unsafe-perm.
printf "unsafe-perm=true\n" >> .npmrc

# get dependencies
npm install

# https://stackoverflow.com/questions/1221833/pipe-output-and-capture-exit-status-in-bash
try() {
  echo "[executing: $*]"
  # shellcheck disable=SC2162 disable=SC2086
  ( ( ( ("$@" 2>&1; echo $? >&3) | tee t.tmp >&4) 3>&1) | (read xs; exit $xs)) 4>&1
}

# now really use node-pre-gyp to publish. use npx to invoke
# node-pre-gyp directly so that the output is available to get
# the packaged_file and the published_file.
try npx node-pre-gyp configure || exit 1
try npx node-pre-gyp rebuild || exit 1
try npx node-pre-gyp package || exit 1
packaged_file=$(sed -n 's/^.*Binary staged at "\(.*\)"$/\1/p' < t.tmp)
echo "[packaged file ${packaged_file}]"

# [@appoptics/apm-bindings] published to https://apm-bruce-staging.s3.amazonaws.com/apm_bindings/v11.0.0/apm_bindings-v11.0.0-napi-v4-glibc-x64.tar.gz
try npx node-pre-gyp publish --s3_host="$s3_host" || exit 1
published_file=$(sed -n 's/^\[@appoptics\/apm-bindings\] published to \(.*\)$/\1/p' < t.tmp)

echo "[published file ${published_file}]"

echo "::set-output name=branch::$branch"

echo "::set-output name=publish-name::${packaged_file}"
echo "::set-output name=publish-destination::${published_file} ($s3_host)"
