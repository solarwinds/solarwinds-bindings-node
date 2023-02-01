FROM amazonlinux:2022

ENV NODE_VERSION 18.13.0

# install software required for this OS
RUN yum -y install \
    tar \
    gzip

# install nvm
ENV NVM_DIR /root/.nvm

# install nvm using a pre-vetted script
COPY nvm-v0.39.3-install.sh /
RUN bash nvm-v0.39.3-install.sh

ENV NODE_PATH /root/.nvm/v$NODE_VERSION/lib/node_modules
ENV PATH /root/.nvm/versions/node/v$NODE_VERSION/bin:$PATH

# install node and npm
RUN source $NVM_DIR/nvm.sh \
    && nvm install $NODE_VERSION \
    && nvm alias default $NODE_VERSION \
    && nvm use default

# add node and npm to path so the commands are available
ENV NODE_PATH $NVM_DIR/v$NODE_VERSION/lib/node_modules
ENV PATH $NVM_DIR/versions/node/v$NODE_VERSION/bin:$PATH

# make node available to all users
RUN chmod -R a+rx $NVM_DIR
