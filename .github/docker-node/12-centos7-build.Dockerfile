FROM centos:7

ENV NODE_VERSION 12.22.1

# centos needs the user to be root; sudo doesn't work.
USER root

# install software required for this OS
RUN yum -y install \
  gcc-c++ \
  python2 \
  make

# install nvm
ENV NVM_DIR /root/.nvm

# install nvm using a pre-vetted script
COPY nvm-v0.38.0-install.sh /
RUN bash nvm-v0.38.0-install.sh

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
