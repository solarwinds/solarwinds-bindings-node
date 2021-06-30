FROM centos:7

ENV NODE_VERSION 16.3.0

# centos needs the user to be root; sudo doesn't work.
USER root
# yum returns non-zero exit code (100) if packages available for update
RUN yum -y check-update || echo "packages available for update"

# install software required for this OS
RUN yum -y install \
  python2 \
  make \
  git \
  curl \
  which \
  nano

# yum install gcc-c++  will install version 4.8.5
# we need a higher version that supports c++ 14.
# following will get version 7.3.1 and set as default
RUN yum install -y centos-release-scl
RUN yum install -y devtoolset-7-gcc*

ENV PATH=/opt/rh/devtoolset-7/root/bin:$PATH

# install nvm
ENV NVM_DIR /root/.nvm


RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.37.1/install.sh | bash

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
