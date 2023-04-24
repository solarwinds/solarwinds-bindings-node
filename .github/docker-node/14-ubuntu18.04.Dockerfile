FROM ubuntu:18.04

ENV NODE_VERSION 14.21.3

# install software required for this OS
RUN apt-get update && apt-get install -y curl

# install nvm
ENV NVM_DIR /root/.nvm

# install nvm using a pre-vetted script
COPY nvm-v0.39.3-install.sh /
RUN bash nvm-v0.39.3-install.sh

ENV NODE_PATH /root/.nvm/v$NODE_VERSION/lib/node_modules
ENV PATH /root/.nvm/versions/node/v$NODE_VERSION/bin:$PATH

# install node and npm
RUN . $NVM_DIR/nvm.sh \
    && nvm install $NODE_VERSION \
    && nvm alias default $NODE_VERSION \
    && nvm use default

# add node and npm to path so the commands are available
ENV NODE_PATH $NVM_DIR/v$NODE_VERSION/lib/node_modules
ENV PATH $NVM_DIR/versions/node/v$NODE_VERSION/bin:$PATH

# make node available to all users
RUN chmod -R a+rx $NVM_DIR
