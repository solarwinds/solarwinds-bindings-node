# Container that will execute the tests
ARG image
FROM $image

ARG script_to_run
ARG branch
ARG token
ARG workspace
ARG node_version
ARG os_string
ENV SCRIPT_TO_RUN=$script_to_run \
    BRANCH=$branch \
    TOKEN=$token \
    GITHUB_ACTIONS=true \
    CI=true \
    GITHUB_WORKSPACE=$workspace \
    NODE_VERSION=$node_version \
    OS_STRING=$os_string

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
ENTRYPOINT /image-scripts/common.sh "$SCRIPT_TO_RUN" $BRANCH $TOKEN $NODE_VERSION $OS_STRING
