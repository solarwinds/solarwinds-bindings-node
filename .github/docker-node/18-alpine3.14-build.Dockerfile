FROM node:18-alpine3.14

# install software required for this OS
RUN apk update && apk add \
  g++ \
  python3 \
  make
