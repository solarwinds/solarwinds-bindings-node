# Container that will execute the tests
ARG image
FROM $image

ARG SCRIPT_TO_RUN
ARG branch
ARG token
ARG workspace
ARG node_version
ARG os_string

ENV SCRIPT_TO_RUN=$SCRIPT_TO_RUN \
    BRANCH=$branch \
    TOKEN=$token \
    GITHUB_ACTIONS=true \
    CI=true \
    GITHUB_WORKSPACE=$workspace \
    NODE_VERSION=$node_version \
    OS_STRING=$os_string

# centos needs the user to be root; sudo doesn't work.
USER root
# yum returns non-zero exit code (100) if packages available for update
RUN yum -y check-update || echo "packages available for update"

# install software required for this OS
RUN yum -y install \
  gcc-c++ \
  make \
  git \
  curl \
  tar \
  nano

RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.37.1/install.sh | bash

RUN mkdir /image-scripts
COPY *.sh /image-scripts/
RUN chmod +x /image-scripts/*

# use no brackets so the env vars are interpreted
ENTRYPOINT /image-scripts/common.sh "$SCRIPT_TO_RUN" $BRANCH $TOKEN $NODE_VERSION $OS_STRING
