#!/bin/sh -l

branch="$1"
# shellcheck disable=SC2034 # used by appoptics-bindings in tests
export AO_TOKEN_PROD="$2"
node_version="$3"
# shellcheck disable=SC2034
os_string="$4"

# make os-release one line and get rid of the garbage that confuses github actions
details=$(tr -d '()' < /etc/os-release | tr '\n' ',' | sed 's/ANSI_COLOR="0;3.",//')
echo "::set-output name=os-details::$details"

# if node isn't installed them nvm should be (handled by <os>.Dockerfile)
if ! which node; then
    [ -d "$HOME/.nvm" ] || exit 1
    export NVM_DIR="$HOME/.nvm"
    # shellcheck disable=SC1090
    [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"
    nvm install "$node_version"
fi

cd "$GITHUB_WORKSPACE" || exit 1
git clone --depth=1 https://github.com/appoptics/appoptics-bindings-node aob -b "$branch"
cd aob || exit 1

# make sure production install works
if ! npm install --production --unsafe-perm; then
  echo "::set-output name=install-prod::fail"
  error=true
else
  echo "::set-output name=install-prod::pass"
fi

rm -rf node_modules

# install not production so testing works
if ! npm install --unsafe-perm; then
  echo "::set-output name=install-dev::fail"
  error=true
else
  echo "::set-output name=install-dev::pass"
fi

# mocha is installed globally in the development environment; it's
# not a devDependency.
npm install -g mocha || error=true

#
# setup some vars that the test script will look for
# and use.
#
export AOBT_PASS_COUNT="::set-output name=tests-passed-count::"
export AOBT_FAIL_COUNT="::set-output name=tests-failed-count::"
export AOBT_FAILED_TESTS="::set-output name=tests-failed::"

# now execute the test
if ! npm test; then
  echo "::set-output name=install-dev::fail"
  error=true
else
  echo "::set-output name=install-dev::pass"
fi

if [ -n "$error" ]; then
  exit 1
fi

exit 0
