#!/bin/sh
# note: alpine has no bash
#
# to run the image:
# ./docker_dev [node version, defaults to 14]
#
# Can use specific tags:
# ./docker_dev 12
# ./docker_dev 12-alpine3.11

ver=${1:-14}

# remove artifacts left locally by previous npm install
rm -rf node_modules 
rm -rf dist

# pull a standard image
docker pull node:$ver

# mount local dir to container
# and install what is needed
docker run -it \
    --privileged \
    --workdir /usr/src/appoptics-bindings-node \
    -v `pwd`:/usr/src/appoptics-bindings-node \
    node:$ver sh -c "npm install --unsafe-perm"

# open a shell
docker run -it \
    --hostname "node-${ver}" \
    --privileged \
    --workdir /usr/src/appoptics-bindings-node \
    -v `pwd`:/usr/src/appoptics-bindings-node \
    --env-file .env \
    node:$ver sh
