FROM node:16-alpine3.11

# install software required for this OS
RUN apk update && apk add \
  g++ \
  python2 \
  make