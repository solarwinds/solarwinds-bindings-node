#!/bin/sh

#
# examples:
# docker run -it -v $PWD/logs:/root/appoptics-apm-node/logs node-8-ubuntu-matrix /bin/bash
# docker run -d -v $PWD/logs:/root/appoptics-apm-node/logs node-8-ubuntu-matrix
#
# of course, each log directory could be a different location based on either node version,
# os flavor, or both.
#
[ -d "$PWD/output/" ] || { echo "ERROR: $PWD/output/ must be mapped in the docker run command"; exit 1; }

# checkout the right branch
git checkout "$AO_BRANCH"

# if PULL is not empty then pull
[ -z "$PULL" ] || git pull

# install the dependencies
npm install || cat npm-debug.log

# setup the oboe links
node setup-liboboe.js

#
# verify that an install works
#
#npm pack
#npmpack=$(ls *.tgz | tail -1)
#mkdir -p ../test-install
#mv $npmpack ../test-install
#olddir=$PWD
#cd ../test-install
#npm install ./$npmpack > install.log
#error=$?
#cd $olddir
#if [ $error -ne 0 ]; then
#  mv ../test-install/install.log ./output/npm-install-error-$NODE_VERSION-$OS_SPEC
#  exit $error
#fi

echo "ok, i'm gonna build the bindings now"
npm run rebuild

# now copy the built file to the output directory
cp build/Release/appoptics-bindings.node ./output
