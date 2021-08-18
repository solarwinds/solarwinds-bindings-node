FROM ubuntu:20.04
SHELL ["/bin/bash", "-c"]

# general tools
RUN apt update \
    && apt -y install \
                curl \
                git \
                nano \
                g++ \
                python \
                make

# set time zone (for github cli)
ENV TZ=America/Los_Angeles
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# for github cli
RUN apt -y install \
            gnupg \
            software-properties-common \
            tzdata

# get and install github cli
# see: https://github.com/cli/cli/issues/1797#issuecomment-696469523
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-key C99B11DEB97541F0 \
    && apt-add-repository https://cli.github.com/packages \
    && apt -y install gh

# install nvm
ENV NVM_DIR /root/.nvm

RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.38.0/install.sh | bash

# these are the stable versions as of July 2021
# cant use lts alias due to all sorts of Dockerfile limitations.
RUN source $NVM_DIR/nvm.sh \
    && nvm install v14.17.5 \
    && nvm install v12.22.5 \
    && nvm install v10.24.1 \
    && nvm install stable

# currently package-lock lockfileVersion is 1
# stay with 14.

# add node and npm to path so the commands are available
ENV NODE_PATH $NVM_DIR/v14.17.5/lib/node_modules
ENV PATH $NVM_DIR/versions/node/v14.17.5/bin:$PATH
