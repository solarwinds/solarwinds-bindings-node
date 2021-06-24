#!/bin/bash

os_node="$1"
aws_access_key_id="$2"
aws_secret_access_key="$3"

set -e

# shellcheck disable=SC2076
if [[ $os_node =~ ".github" ]]; 
then
  echo "Container from File"
  docker build -t my:tag "$os_node"
  container_id=$(docker run \
    -t -d --privileged \
    -e AO_TOKEN_PROD="$AO_TOKEN_PROD" \
    -e AWS_ACCESS_KEY_ID="$aws_access_key_id" \
    -e AWS_SECRET_ACCESS_KEY="$aws_secret_access_key" \
    --workdir=/usr/src/work/ \
    my:tag)
else
  echo "Container from Image"
  docker pull "$os_node"
  container_id=$(docker run \
    -t -d --privileged \
    -e AO_TOKEN_PROD="$AO_TOKEN_PROD" \
    --workdir=/usr/src/work/ \
    "$os_node")
fi

echo "Copy files"
docker cp ./. "$container_id":/usr/src/work/

# just for show
echo "Show system info"
echo "Container Id is ""$container_id"""
docker exec "$container_id" printenv
docker exec "$container_id" node --version
docker exec "$container_id" npm --version
docker exec "$container_id" cat /etc/os-release
docker exec "$container_id" pwd
docker exec "$container_id" ls -al

# return a value
echo "::set-output name=containerId::$container_id"
