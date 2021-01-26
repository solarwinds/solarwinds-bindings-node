# Container that will execute the tests
ARG image
FROM $image

ARG script_to_run
ARG branch
ARG token
ARG workspace
ARG node_version
ARG os_string
# the following are used for publish.sh but not node-os-tests.sh
ARG AWS_ACCESS_KEY_ID
ARG AWS_SECRET_ACCESS_KEY
ARG PRODUCTION

# docker doesn't do arg substitutions on the strings for ENTRYPOINT so
# these all need to be env vars. the shell does those substitutions.
ENV SCRIPT_TO_RUN=$script_to_run \
    BRANCH=$branch \
    TOKEN=$token \
    GITHUB_ACTIONS=true \
    CI=true \
    GITHUB_WORKSPACE=$workspace \
    NODE_VERSION=$node_version \
    OS_STRING=$os_string \
    AWS_ACCESS_KEY_ID="$AWS_ACCESS_KEY_ID" \
    AWS_SECRET_ACCESS_KEY="$AWS_SECRET_ACCESS_KEY" \
    PRODUCTION="$PRODUCTION"

# install software required for this OS
RUN apk update && apk add \
  g++ \
  python2 \
  make \
  git \
  curl \
  nano

RUN mkdir /image-scripts
COPY *.sh /image-scripts/
RUN chmod +x /image-scripts/*

# use no brackets so the env vars are interpreted
ENTRYPOINT /image-scripts/common.sh "$SCRIPT_TO_RUN" \
    "$BRANCH" \
    "$TOKEN" \
    "$NODE_VERSION" \
    "$OS_STRING" \
    "$AWS_ACCESS_KEY_ID" \
    "$AWS_SECRET_ACCESS_KEY" \
    "$PRODUCTION"
