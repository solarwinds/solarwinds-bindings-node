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

# install software required for this OS
RUN apk update && apk add \
  g++ \
  python2 \
  make \
  git \
  curl \
  nano


COPY build-and-test-bindings.sh /build-and-test-bindings.sh
RUN chmod +x /build-and-test-bindings.sh

# use the no brackets for so the env vars are interpreted
ENTRYPOINT /build-and-test-bindings.sh $BRANCH $TOKEN $NODE_VERSION $OS_STRING
