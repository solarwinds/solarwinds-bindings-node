# Contributing

## Two minutes on how it works

[node-appoptics-bindings](https://github.com/librato/node-appoptics-bindings)
implements the low-level interface to liboboe. liboboe implements core functions
to enable tracing. Traces are sequences of entry and exit events which capture
performance information. Each event has a:

```
label: (string - typically entry or exit)
layer: (string - the name of the layer being instrumented)
timestamp: (integer - unix timestamp)
hostname: (string)
X-trace ID: <header><task ID><op ID><flags>
edge: the previous event's X-trace ID
```

An X-trace ID (XID) comprises a single byte header with the hex value `2B`, a task ID
of 20 bytes that is constant for all events in a trace, an op ID of 8 bytes that
is unique for each event in a trace, a flag byte. In this implementation the XID is
often manipulated as a string of hex digits, so the task ID is 40 characters, the
op ID is 16 characters, and the header and flags are two characters each.




## Dev environment

Like [node-appoptics](http://github.com/librato/node-appoptics), this repo
contains a `Vagrantfile` to build a dev environment VM. This one is simpler
though, due to not needing database docker containers.

Similarly, it's just a matter of running `vagrant up` and `vagrant ssh` to
start the VM and connect to it.

## Testing

This repo is simpler than `node-appoptics-bindings` in that it doesn't have
a segmented test suite. Simply execute `gulp test` or `npm test` to run the test suite.

The following environment variables must be set for the "should get verification that a request should be sampled" test.

- `APPOPTICS_COLLECTOR=collector-stg.appoptics.com:4444` or `:443`
- `APPOPTICS\_SERVICE_KEY=795fb4947d15275d208c49cfd2412d4a5bf38742045b47236c94c4fe5f5b17c7:name`

TODO: extend mock reporter to return correct values to pass this test.

## Project layout

The `src` directory contains the C++ code to bind to liboboe. The `test`
directory contains the test suite.

## Process

### Building

Whenever you do an `npm install`, the build process will be run automatically.
This method uses a `prepublish` script in `package.json` to conceal compiler
output though, so you may want to skip that if dealing with build issues. To
do so, you can use `npm run rebuild` to bypass the `prepublish` script and
run the `node-gyp rebuild` phase directly. You can also set the environment
variable `APPOPTICS\_SHOW_GYP` to any non-empty value.

### Developing

Just like `node-appoptics`, this repo uses simple feature branches that only
get merged to master at release time.

### Releasing

Again, like `node-appoptics` the `npm version patch|minor|major` tool is used
to create a version bump commit and tag to push to github, then you can simply
run `npm publish` to publish the release.
