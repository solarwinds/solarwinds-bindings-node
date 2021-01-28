#!/bin/sh -l

# this is the entrypoint of generic.Dockerfile. It will build an os/node
# specific container that will run the tests.
#
# NODE_IMAGE is an encoded specification for the image:tag[+node:node-version]
# 'node:10-alpine3.9'
# 'centos:7+node:10'
#
# the first is used when are pre-built node image can be used; the second is
# used when there is not an image pre-built with node that can be used. those
# images install node in their dockerfiles.
#
SCRIPT_TO_RUN=$1
NODE_IMAGE=$2
BRANCH=$3         # the branch to test
TOKEN=$4          # the AO_SWOKEN to use for the tests//access-enabled url for publishing

#
# the following positional arguments are only supplied by the publish workflow.
#
AWS_ACCESS_KEY_ID=$5
AWS_SECRET_ACCESS_KEY=$6
PRODUCTION=$7     # default false implies staging; specify true for production

echo "::set-output name=all-args-generic::$*"

# get the image and, if centos, the node version specification
IFS='+' read -r image node_spec << EOS
$NODE_IMAGE
EOS

# split the image into the base and the tag
IFS=':' read -r base tag << EOS
$image
EOS

# this only applies to node: base images. split them into
# node-version and os-version.
IFS='-' read -r tag_node_version tag_os_version << EOS
$tag
EOS

# this only applies to images that aren't node: base images.
# split them into "node" and the node-version.
# shellcheck disable=SC2034
IFS=':' read -r node_key key_node_version << EOS
$node_spec
EOS

#
# if base is node then 1) node is installed, 2) the os is "$tag_os_version",
# and 3) the node version is "$tag_node_version".
#
# if the base is not node then 1) node is not installed, 2) the os is
# "$base$tag", and 3) the node version is "$key_node_version".
#
# in all cases, the image to use is "$image"
#
if [ "$base" = "node" ]; then
  os_string="$tag_os_version"
  node_version="$tag_node_version"
else
  os_string="$base$tag"
  node_version="$key_node_version"
fi

# get dockerfile selection
case "$os_string" in
  alpine*) os=alpine ;;
  stretch*|buster*) os=debian ;;
  centos*) os=centos ;;
  amazonlinux*) os=amazonlinux ;;
  # if nothing matches then this script needs to be updated
  *) echo "unknown operating system $os_string" && exit 1 ;;
esac

#echo "base: $base"
#echo "image: $image"
#echo "node_version: $node_version"
#echo "os_string: $os_string"
#echo "os: $os"
#
#exit 0

cd /image-actions || exit 1
echo "creating test image from: $image"
echo "make a change"

# here we can make the construction of the image as customizable as we need.
# if we need parameterizable values add them as arguments.
docker build . -f "$os.Dockerfile" -t "docker-$os_string-$node_version" \
    --build-arg workspace="$GITHUB_WORKSPACE" \
    --build-arg image="$image" \
    --build-arg script_to_run="$SCRIPT_TO_RUN" \
    --build-arg branch="$BRANCH" \
    --build-arg token="$TOKEN" \
    --build-arg node_version="$node_version" \
    --build-arg os_string="$os_string" \
    --build-arg AWS_ACCESS_KEY_ID="$AWS_ACCESS_KEY_ID" \
    --build-arg AWS_SECRET_ACCESS_KEY="$AWS_SECRET_ACCESS_KEY" \
    --build-arg PRODUCTION="$PRODUCTION" \
    && docker run -v /github/workspace:/github/workspace "docker-$os_string-$node_version"
