FROM node:17-alpine3.12

# install software required for this OS
RUN apk update && apk add \
  g++ \
  python3 \
  make
