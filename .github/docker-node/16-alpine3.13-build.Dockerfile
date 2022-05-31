FROM node:16-alpine3.13

# install software required for this OS
RUN apk update && apk add \
  g++ \
  python3 \
  make
