FROM node:14-alpine3.10

# install software required for this OS
RUN apk update && apk add \
  g++ \
  python2 \
  make