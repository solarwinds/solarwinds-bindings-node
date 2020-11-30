# Container that will execute the tests
ARG image
FROM $image

ARG branch
ARG token
ARG workspace
ARG node_version
ARG os_string

ENV BRANCH=$branch \
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

COPY build-and-test-bindings.sh /build-and-test-bindings.sh
RUN chmod +x /build-and-test-bindings.sh

# use the no brackets for so the env vars are interpreted
ENTRYPOINT /build-and-test-bindings.sh $BRANCH $TOKEN $NODE_VERSION $OS_STRING
