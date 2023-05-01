FROM ubuntu:22.04
SHELL ["/bin/bash", "-c"]

# general tools
RUN apt-get update && apt-get -y install curl git

# set time zone (for github cli)
ENV TZ=America/Vancouver
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# for github cli
RUN apt-get -y install \
            gnupg \
            software-properties-common \
            tzdata

# get and install github cli
RUN curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg \
    && chmod go+r /usr/share/keyrings/githubcli-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | tee /etc/apt/sources.list.d/github-cli.list > /dev/null \
    && apt update \
    && apt install gh -y

# install nvm
ENV NVM_DIR /root/.nvm

RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.3/install.sh | bash

# these are the stable versions as of January 2022
# can't use lts alias due to all sorts of Dockerfile limitations.
RUN source $NVM_DIR/nvm.sh \
    && nvm install v20.0.0 \
    && nvm install v18.16.0 \
    && nvm install v16.20.0 \
    && nvm install stable

# add node and npm to path so the commands are available
ENV NODE_PATH $NVM_DIR/v20.0.0/lib/node_modules
ENV PATH $NVM_DIR/versions/node/v20.0.0/bin:$PATH
