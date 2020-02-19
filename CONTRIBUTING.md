# Contributing

## Two minutes on how it works

[appoptics-bindings](https://github.com/appoptics/appoptics-bindings-node)
implements the low-level interface to liboboe. liboboe implements communications and aggregation functions
to enable the efficient sampling of traces. Traces are sequences of entry and exit events which capture
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

Note: layer is a legacy term for span. It has been replaced in the code but still appears in event messages as Layer and in the liboboe interface.


## Dev environment

The development environment is quite simple. Only the tools to build the
extension (the gcc tool suite version 4.7 or above) and Python 2.7 for node-gyp
are required.

The sourceable script file `env.sh` has options to configure the development environment
for different purposes. There is also an option to download a new version of oboe.

Generally the command `$ . env.sh prod` is the most useful - it sets up the environment
variables to work against the real appoptics collector. This is used during the test suite
to verify that basic connectivity exists. It does require `APPOPTICS_SERVICE_KEY` be defined
with a valid service key.


## Testing

This repo is simpler than `appoptics-apm` in that it does not have a segmented test suite.
Simply execute `npm test` to run the test suite.

The following environment variables must be set for the "should get verification that a request should be sampled" test. `env.sh` will set them up for you if you have defined AO_TOKEN_STG

- `APPOPTICS_COLLECTOR=collector.appoptics.com:443` or `:4444`
- `APPOPTICS\_SERVICE_KEY=<a valid service key for the appoptics>:name`

## Debugging

Debugging node addons is not intuitive but this might help (from [stackoverflow](https://stackoverflow.com/questions/23228868/how-to-debug-binary-module-of-nodejs))


First, compile your add-on using node-gyp with the --debug flag.

`$ node-gyp --debug configure rebuild`

(The next point about changing the require path doesn't apply to appoptics-bindings because it uses the `bindings` module and that will find the module in `Debug`, `Release`, and other locations.)

Second, if you're still in "playground" mode like I am, you're probably loading your module with something like

`var ObjModule = require('./ObjModule/build/Release/objModule');`

However, when you rebuild using node-gyp in debug mode, node-gyp throws away the Release version and creates a Debug version instead. So update the module path:

`var ObjModule = require('./ObjModule/build/Debug/objModule');`

Alright, now we're ready to debug our C++ add-on. Run gdb against the node binary, which is a C++ application. Now, node itself doesn't know about your add-on, so when you try to set a breakpoint on your add-on function (in this case, StringReverse) it complains that the specific function is not defined. Fear not, your add-on is part of the "future shared library load" it refers to, and will be loaded once you require() your add-on in JavaScript.

```
$ gdb node
...
Reading symbols from node...done.
(gdb) break StringReverse
Function "StringReverse" not defined.
Make breakpoint pending on future shared library load? (y or [n]) y
```

OK, now we just have to run the application:

```
(gdb) run ../modTest.js
...
Breakpoint 1, StringReverse (args=...) at ../objModule.cpp:49
```

If a signal is thrown gdb will stop on the line generating it.

Finally, here's a link to using output formats (and the whole set of gdb docs) [gdb](http://www.delorie.com/gnu/docs/gdb/gdb_55.html).

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

(This section will not be useful for installing once the N-API version has been
released.)


### Developing

Just like `appoptics-apm`, this repo uses simple feature branches that only
get merged to master via a pull request.

### Releasing

Again, like `appoptics-apm` the `npm version major.minor.patch` tool is used
to commit a version bump and tag to push to github. You must use `git push origin 'tagname'` in order to push the tag in addition to the changes. If you do not the tag will not be sent to the repo. The versioning should follow [semver](www.semver.org) conventions, most importantly that breaking changes get a major version update. A pull request should be generated on github. Once the PR has been merged to master on github then, after updating local master with `git pull`, you can run `npm publish` to publish the release. This should be done using a `@solarwinds.cloud` email account. The branch can be deleted once it has been merged.

### Miscellaneous

(Use tail if you only want to see the highest version required, leave it off to see all.)

Find the highest version of GLIBCXX is supported in /usr/lib/libstdc++.so.?

`readelf -sV /usr/lib/libstdc++.so.6 | sed -n 's/.*@@GLIBCXX_//p' | sort -u -V | tail -1`

Find the versions of GLIBCXX required by a file

`readelf -sV build/Release/appoptics-bindings.node | sed -n 's/^.*\(@GLIBCXX_[^ ]*\).*$/\1/p' | sort -u -V`
`objdump -T /lib/x86_64-linux-gnu/libc.so.6 | sed -n 's/^.*\(GLIBCXX_[^ ]*\).*$/\1/p' | sort -u -V`

Dump a `.node` file as asm (build debug for better symbols):

`objdump -CRrS build/Release/ao-metrics.node  > ao-metrics.s`
