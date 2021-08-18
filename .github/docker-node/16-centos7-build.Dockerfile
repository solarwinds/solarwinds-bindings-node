FROM centos:7

ENV NODE_VERSION 16.6.1

# install software required for this OS
RUN yum -y install \
  python2 \
  make

# yum install gcc-c++  will install version 4.8.5
# we need a higher version that supports c++ 14.
# see: https://github.com/nodejs/node/blob/master/BUILDING.md#supported-toolchains
# following will get version 8.3.1 and set as default
RUN yum install -y centos-release-scl
RUN yum install -y devtoolset-8-gcc*

ENV PATH=/opt/rh/devtoolset-8/root/bin:$PATH

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
