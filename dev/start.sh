#!/bin/bash

# script used to enable local mac dev.
# trigger via npm run.
# no other usage.
# always WIP.

# check that the docker engine is running
docker ps > /dev/null || exit

cleanup() {
    # remove artifacts left locally by previous npm install
    rm -rf build
    rm -rf node_modules 
    rm -rf dist
    rm -rf build-tmp-napi-v7
    rm -rf build-tmp-napi-v4

    docker stop "$container_id"
    docker rm "$container_id"
}

set -e
trap cleanup EXIT

# silently cleanup from past aborted launches
docker rm -f /dev-bindings &> /dev/null

docker build dev/. -t dev-bindings

# open a shell in detached mode
container_id=$(docker run -itd \
    --hostname dev-bindings \
    --name dev-bindings \
    --privileged \
    --workdir /usr/src/work \
    -v "$(pwd)":/usr/src/work \
    -v "$(pwd)/dev/volumes/tmp/appoptics":/tmp/appoptics \
    -v ~/.gitconfig:/root/.gitconfig \
    -v ~/.ssh:/root/.ssh \
    --env-file .env \
    dev-bindings bash)

docker exec "$container_id" npm install --unsafe-perm

# ready for work
docker attach "$container_id"
