FROM centos/nodejs-12-centos7

USER root

RUN yum -y check-update || echo "packages available for update"

RUN yum -y install \
  gcc-c++ \
  python2 \
  make \
  git \
  curl \
  nano

COPY ./build-and-test-bindings.sh /root/build-and-test-bindings.sh
RUN chmod +x /root/build-and-test-bindings.sh

ENTRYPOINT ["/root/build-and-test-bindings.sh"]



