# Contributing

## Dev environment

Like [node-traceview](http://github.com/appneta/node-traceview), this repo
contains a `Vagrantfile` to build a dev environment VM. This one is simpler
though, due to not needing database docker containers.

Similarly, it's just a matter of running `vagrant up` and `vagrant ssh` to
start the VM and connect to it.

## Testing

This repo is simpler in that it doesn't have a segmented test suite. You
can simply run `gulp test` or `npm test` to run the test suite.

## Project layout

The `src` directory contains the C++ code to bind to liboboe. The `test`
directory contains the test suite.

## Process

### Building

Whenever you do an `npm install`, the build process will be run automatically.
This method uses a `prepublish` script in `package.json` to conceal compiler
output though, so you may want to skip that if dealing with build issues. To
do so, you can use `npm run rebuild` to bypass the `prepublish` script and
run the `node-gyp rebuild` phase directly.

### Developing

Just like `node-traceview`, this repo uses simple feature branches that only
get merged to master at release time.

### Releasing

Again, like `node-traceview` the `npm version patch|minor|major` tool is used
to create a version bump commit and tag to push to github, then you can simply
run `npm publish` to publish the release.
