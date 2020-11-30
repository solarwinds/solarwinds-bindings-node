# this container runs 'entrypoint.sh' to build another container, because it's
# not possible to dynamically specify a docker file or an image in github actions.
FROM alpine:latest

# Copy the docker files that we'll build.
COPY docker-action /docker-action
COPY entrypoint.sh /entrypoint.sh

RUN apk add --update --no-cache docker
RUN chmod +x /entrypoint.sh

# Code file to execute when the docker container starts up (`entrypoint.sh`)
ENTRYPOINT ["/entrypoint.sh"]
