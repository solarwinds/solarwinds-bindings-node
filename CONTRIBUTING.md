# Contributing

## Two minutes on how it works

[@appoptics/apm-bindings](https://github.com/appoptics/appoptics-bindings-node)
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

The tools to build the extension (gcc tool suite version 4.7 or above and Python 2.7) for node-gyp
are required.

Linting is not currently part of the build process. Invoke `npm run eslint` to check for errors.

The sourceable script file `env.sh` has options to configure the development environment
for different purposes. There is also an option to download a new version of oboe.

Generally the command `. env.sh prod` is the most useful - it sets up the environment
variables to work against the real appoptics collector. This is used during the test suite
to verify that basic connectivity exists. It does require `APPOPTICS_SERVICE_KEY` be defined
with a valid service key.

## Testing

Execute `npm test` to run the test suite.

The following environment variables must be set for the "should get verification that a request should be sampled" test. `env.sh` will set them up for you if you have defined AO_TOKEN_PROD.

- `APPOPTICS_COLLECTOR=collector.appoptics.com:443`
- `APPOPTICS_SERVICE_KEY=<a valid service key for the appoptics>:name`

## With Docker

1. Create a `.env` file and set: `AO_TOKEN_PROD={a valid production token}`. Potentially you can also set `AO_TOKEN_STG={a valid staging token}`
2. Run `./docker-dev.sh` - this will create a docker container, set it up, and open a shell. To specify a node version run `./docker-dev.sh {version}`
3. Run `npm test` to make sure all is ok. To run tests in other configurations use `. env.sh stg`, `. env.sh udp`

## Debugging

Debugging node addons is not intuitive but this might help (from [stackoverflow](https://stackoverflow.com/questions/23228868/how-to-debug-binary-module-of-nodejs))


First, compile your add-on using node-gyp with the --debug flag.

`node-gyp --debug configure rebuild`

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
and its required header files. `.github` contains the files for github
actions.

## Process

### Building

`setup-liboboe.js` must be run in order to create symbolic links to the correct version
of liboboe so the `SONAME` field can be satisfied. The `install` and `rebuild` scripts
run `setup-liboboe.js` as the first step. `setup-liboboe.js` can be run multiple times.

`node-pre-gyp` is now used so that in most cases end users will not need to install the
build tool chain. For development you will either need to change the install script so
that it uses `--build-from-source` instead of `--fallback-to-build` or you can change the
`binary.staging_host` property to point to an S3 bucket that you can publish too. If
changing the `binary.staging_host` you'll need to prefix the install command by setting
`node_pre_gyp_s3_host=staging` so that it installs from the staging host, e.g.,
`node_pre_gyp_s3_host=staging node-pre-gyp install --fallback-to-build`. The reason is that
publishing defaults to staging while installation defaults to production.


### Developing

Just like `appoptics-apm`, this repo uses simple feature branches that get merged to master
via a pull request. Developers without write access to the repository should fork the repo and
submit a PR.

### Releasing

Before releasing the version in package.json should be bumped using [semver](www.semver.org) conventions.
Then a git tag with the same version (but with a `v` prefix) should be created. Push the tag using
`git push origin tagname` or it will not be sent to the repo.

Before publishing switch to the master branch and execute `git pull` to make sure what will be published
is in sync with the repository. Publishing is done from the master branch.

For releasing there are two steps. The github action `publish` is used to publish pre-built binaries via
node-pre-gyp. npm is then used to publish to `npmjs` using the command `npm publish --access public --otp=123456`.
If publishing an alpha/rc release you *must* add `--tag=alpha` (or `rc` or `rc1`) so that it will not become
the default download. Publishing should be done using an account with a Solarwinds email.

The reason that these two steps are decoupled is that it's possible for the node-pre-gyp publish step to fail
even when `npm publish` to npmjs.com succeeds. It is not possible to replace a version published to npmjs.com, so
publishing each separately avoids extra version bumps if a node-pre-gyp error occurs. When developing, you might
want to manually publish using node-pre-gyp; the steps to do that are in
`.github/actions/generic/image-actions/publish.sh`. while node-pre-gyp will not publish on top of an existing file by the same name, it does allow unpublishing; the S3 console can also be used to delete the published files.

By default the `publish` command will publish to `staging_host` while the `install` command will install from
`production_host`. This facilitates testing while making it seamless for end users to install the production code.
To publish to `production_host` preface use the option `--s3_host=production` or set the environment variable
`node_pre_gyp_s3_host=production`. Set either to `staging` to make both commands target `staging_host`.


### Miscellaneous

(Use tail if you only want to see the highest version required, leave it off to see all.)

Find the highest version of GLIBCXX is supported in /usr/lib/libstdc++.so.?

`readelf -sV /usr/lib/libstdc++.so.6 | sed -n 's/.*@@GLIBCXX_//p' | sort -u -V | tail -1`

Find the versions of GLIBCXX required by a file

`readelf -sV build/Release/appoptics-bindings.node | sed -n 's/^.*\(@GLIBCXX_[^ ]*\).*$/\1/p' | sort -u -V`
`objdump -T /lib/x86_64-linux-gnu/libc.so.6 | sed -n 's/^.*\(GLIBCXX_[^ ]*\).*$/\1/p' | sort -u -V`

Dump a `.node` file as asm (build debug for better symbols):

`objdump -CRrS build/Release/ao-metrics.node  > ao-metrics.s`
