ARG IMAGE
FROM $IMAGE

ARG OS_SPEC
ARG NODE_VERSION
ARG TAG
ARG AO_BRANCH
ARG PULL

ENV NODE_VERSION=$NODE_VERSION \
    OS_SPEC=${OS_SPEC} \
    TAG=$TAG \
    AO_BRANCH=$AO_BRANCH \
    PULL=$PULL

# copy and run the script that installs software, including
# the git repository.
ADD ./install-software.sh /root/install-software.sh
RUN /root/install-software.sh

# go to the repo cloned by install-software script.
WORKDIR /root/appoptics-bindings-node

# fetch these last so as many docker steps as possible can
# be reused.
ADD ./build-bindings.sh /root/build-bindings.sh
RUN chmod 744 /root/build-bindings.sh

CMD ["/root/build-bindings.sh"]
