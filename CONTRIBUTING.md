# Contributing

## Two minutes on how it works

[node-appoptics-bindings](https://github.com/librato/node-appoptics-bindings)
implements the low-level interface to liboboe. liboboe implements core functions
to enable tracing. Traces are sequences of entry and exit events which capture
performance information. Each event has a:

```
label: (string - typically entry or exit)
span: (string - the name of the span being instrumented)
timestamp: (integer - unix timestamp)
hostname: (string)
X-trace ID: <header><task ID><op ID><flags>
edge: the previous event's X-trace ID
```

An X-trace ID (XID) comprises a single byte header with the hex value `2B`, a task ID
of 20 bytes that is the same for all events in a trace, an op ID of 8 bytes that
is unique for each event in a trace, and a flag byte. In this implementation the XID is
manipulated as binary data (via Metadata and Event objects) or as a string of 60 hex digits.
As a string, the header is 2 characters, the task ID is 40 characters, the op ID is 16
characters, and the flag byte is 2 characters.

Note: layer is a legacy term for span. It is being replaced but still appears in the code.


## Dev environment

The development environment is quite simple. Only the tools to build the
extension (the gcc tool suite version 4.7 or above) and Python 2.7 for node-gyp
are required.

The sourceable script file `env.sh` has options to configure the development environment
for different purposes. There is also an option to download a new version of oboe.

Generally the command `$ . env.sh ssl` is the most useful - it sets up the environment
variables to work against a real appoptics collector. This is used during the test suite
to verify that basic connectivity exists.


## Testing

This repo is simpler than `appoptics-apm` in that it does not have a segmented test suite.
Simply execute `npm test` to run the test suite.

The following environment variables must be set for the "should get verification that a request should be sampled" test. `env.sh` will set them up for you if you have defined AO_TOKEN_STG

- `APPOPTICS_COLLECTOR=collector.appoptics.com:443` or `:4444`
- `APPOPTICS\_SERVICE_KEY=<a valid service key for the appoptics>:name`


## Project layout

The `src` directory contains the C++ code to bind to liboboe. The `test`
directory contains the test suite. The `oboe` directory contains liboboe
and its required header files.

## Process

### Building

Whenever you do an `npm install`, a `preinstall` script will run that creates
symbolic links to liboboe so the `SONAME` field can be satisfied. Then the
`build.js` script is run to build the addon. By default the script conceals
compiler output; you may want to skip that if dealing with build issues. There
are multiple ways to do so. `npm run rebuild` bypasses the `build.js` script
altogether. You can also set `APPOPTICS\_SHOW_GYP` to any non-empty value.


### Developing

Just like `appoptics-apm`, this repo uses simple feature branches that only
get merged to master at release time.

### Releasing

Again, like `appoptics-apm` the `npm version patch|minor|major` tool is used
to create a version bump commit and tag to push to github, then you can simply
run `npm publish` to publish the release.
