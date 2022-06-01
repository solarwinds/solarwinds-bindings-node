# @appoptics/apm-bindings

[@appoptics/apm-bindings](https://www.npmjs.com/package/@appoptics/apm-bindings) is an NPM package containing a binary node add-on.

The package is installed as a dependency when the AppOptics APM Agent ([appoptics-apm](https://www.npmjs.com/package/appoptics-apm)) is installed. In any install run, AppOptics APM Agent will first attempt to install a prebuilt add-on using [node-pre-gyp](https://github.com/mapbox/node-pre-gyp) and only if that fails, will it attempt to build the add-on from source using [node-gyp](https://github.com/nodejs/node-gyp).

This is a **Linux Only package** with no Mac or Windows support.

# Table of Contents

- [Two minutes on how it works](#two-minutes-on-how-it-works)
- [Local Development](#local-development)
  * [Project layout](#project-layout)
  * [Docker Dev Container](#docker-dev-container)
  * [Repo Packages](#repo-packages)
  * ["One Off" Docker Container](#one-off-docker-container)
  * [Testing](#testing)
  * [Building](#building)
  * [Debugging](#debugging)
  * [Dev Repo](#dev-repo)
- [Development & Release with GitHub Actions](#development--release-with-github-actions)
  * [Overview](#overview)
  * [Usage](#usage)
  * [Maintenance](#maintenance)
  * [Implementation](#implementation)
- [License](#license)

# Two minutes on how it works

The package implements a low-level interface to `liboboe`, a closed-source library maintained by SolarWinds. `liboboe` implements communications and aggregation functions to enable efficient sampling of traces. Traces are sequences of entry and exit events which capture performance information. 


# Local Development

Development **must be done on Linux**.

To setup a development environment on a Mac use a Docker container (see below).

Mac should have:
  * [Docker](https://docs.docker.com/docker-for-mac/install/)
  * [Xcode command line tools](https://developer.apple.com/download/more/?=command%20line%20tools) (simply installed by terminal `git` command)
  * [SSH keys at github](https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh/adding-a-new-ssh-key-to-your-github-account)

Building with `node-gyp` (`via node-pre-gyp`) requires:
* Python (2 or 3 depending on version of npm)
* make
* A proper C/C++ compiler toolchain, like [GCC](https://gcc.gnu.org/)
    
Those are available in the Docker Dev Container.

## Project layout

`git clone` to start.

* `src` directory contains the C++ code to bind to liboboe. 
* `oboe` directory contains `liboboe` and its required header files. `liboboe` is downloaded from: https://agent-binaries.cloud.solarwinds.com/apm/c-lib. Pre-release versions are at: https://agent-binaries.global.st-ssp.solarwinds.com/apm/c-lib
* `test` directory contains the test suite. 
* `.github` contains the files for github actions.
* `dev` directory contains anything related to dev environment


## Docker Dev Container

1. Start the Docker daemon (on a Mac that would be simplest using Docker desktop).
2. Create a `.env` file and set two sets of keys for both backends:
- `APPOPTICS_SERVICE_KEY={a valid service key}`, `APPOPTICS_COLLECTOR={a url of the collector}` and `AO_TEST_PROD_SERVICE_KEY={a valid **production** service key}`.
- `SW_APM_SERVICE_KEY={a valid service key}`, `SW_APM_COLLECTOR={a url of the collector}` and `SW_TEST_PROD_SERVICE_KEY={a valid **production** service key}`.
3. Run `npm run dev`. This will create a docker container, set it up, and open a shell. Docker container will have all required build tools as well as nano installed, and access to GitHub SSH keys as configured. Repo code is **mounted** to the container.
4. To open another shell in same container use: `docker exec -it dev-bindings /bin/bash`

The setup script ensures a "clean" work place with each run by removing artifacts and installed modules on each exit.

## Repo Packages

This repo has a "single" GitHub package named `node` scoped to `appoptics/appoptics-bindings-node` (the repo) which has [multiple tagged images](https://github.com/appoptics/appoptics-bindings-node/pkgs/container/appoptics-bindings-node%2Fnode). 

Those images serve two main purposes:

1. They complement the official node images (https://hub.docker.com/_/node) with specific end-user configurations.
2. They provide the build environments for the multiple variations (os glibc/musl, node version) of the package.

## "One Off" Docker Container

At times it may be useful to set a "one off" docker container to test a specific feature or build.

1. Run `npm run dev:oneoff`. This will create a docker container, set it up, and open a shell. Docker container will have access to GitHub SSH keys as configured. Repo code is **copied** to the container.
2. To specify an image to the "one off" container pass it as argument. For example: run `npm run dev:oneoff node:latest` to get latest official image or `npm run dev:oneoff ghcr.io/appoptics/appoptics-bindings-node/node:14-alpine3.9` to get one of this repo custom images.

## Testing

Test are run using [Mocha](https://github.com/mochajs/mocha).

1. Run `npm test` to run the test suite against the collector specified in the `.env` file (`APPOPTICS_COLLECTOR`).

Note: the initial default initialization test will always run against production collector using `AO_TEST_PROD_SERVICE_KEY` from the .env file.

The `test` script in `package.json` runs `test.sh` which then manages how mocha runs each test file. To run individual tests use `npx mocha`. For example: `npx mocha test/config.test.js` will run the config tests.


## Building

Building is done using [node-pre-gyp](https://github.com/mapbox/node-pre-gyp).

1. Before a build, `setup-liboboe.js` must run at least once in order to create symbolic links to the correct version of liboboe so the `SONAME` field can be satisfied. 
2.  Run `npx node-pre-gyp rebuild`. More granular commands available. See `node-pre-gyp` documentation.

The `install` and `rebuild` scripts in `package.json` run `setup-liboboe.js` as the first step before invoking `node-pre-gyp`. As a result, initial `npm` install will set links as required so skipping directly to step 2 above is possible. That said, `setup-liboboe.js` can be run multiple times with no issues.

## Debugging

Debugging node addons is not intuitive but this might help (from [stackoverflow](https://stackoverflow.com/questions/23228868/how-to-debug-binary-module-of-nodejs))

First, compile your add-on using `node-pre-gyp` with the `--debug` flag.

`node-pre-gyp --debug configure rebuild`

(The next point about changing the require path doesn't apply to appoptics-bindings because it uses the `bindings` module and that will find the module in `Debug`, `Release`, and other locations.)

Second, if you're still in "playground" mode, you're probably loading your module with something like

`var ObjModule = require('./ObjModule/build/Release/objModule');`

However, when you rebuild using `node-pre-gyp` in debug mode, `node-pre-gyp` throws away the Release version and creates a Debug version instead. So update the module path:

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


### Miscellaneous

Note: use `tail` if you only want to see the highest version required, leave it off to see all.

Find the highest version of GLIBCXX is supported in /usr/lib/libstdc++.so.?

`readelf -sV /usr/lib/libstdc++.so.6 | sed -n 's/.*@@GLIBCXX_//p' | sort -u -V | tail -1`

Find the versions of GLIBCXX required by a file

`readelf -sV build/Release/appoptics-bindings.node | sed -n 's/^.*\(@GLIBCXX_[^ ]*\).*$/\1/p' | sort -u -V`
`objdump -T /lib/x86_64-linux-gnu/libc.so.6 | sed -n 's/^.*\(GLIBCXX_[^ ]*\).*$/\1/p' | sort -u -V`

Dump a `.node` file as asm (build debug for better symbols):

`objdump -CRrS build/Release/ao-metrics.node  > ao-metrics.s`

## Dev Repo

The dev repo setup allows to run end-to-end `node-pre-gyp` and npm release process in a development environment.
It also greatly simplifies creating and testing CI integrations such as GitHub Actions.

It contains:
  - dev repo: https://github.com/appoptics/appoptics-bindings-node-dev (private, permissions via AppOptics Organization admin)
  - staging S3 bucket: https://apm-appoptics-bindings-node-dev-staging.s3.us-east-1.amazonaws.com (public, write permissions via SolarWinds admin)
  - production S3 bucket: https://apm-appoptics-bindings-node-dev-production.s3.amazonaws.com (public, write permissions via SolarWinds admin)

The dev repo was cloned from the main repo and setup with the appropriate secrets.

To set the main repo to work with the dev repo:

1. `git remote -v`
2. `git remote add dev git@github.com:appoptics/appoptics-bindings-node-dev.git`
3. `npm run dev:repo:reset`

The script will:
  - Force push all branches and tags to dev repo.
  - Remove the local dev repo and clone a fresh one into a sibling directory. 
  - Modify package.json:
  ```
  "name": "@appoptics/apm-binding-dev",
  "staging_host": "https://apm-appoptics-bindings-node-dev-staging.s3.us-east-1.amazonaws.com",
  "production_host": "https://apm-appoptics-bindings-node-dev-production.s3.amazonaws.com",
  ```
  - Commit updated `package.json` to `master` all branches.

To start fresh on the dev repo run `npm run dev:repo:reset` again.

When running a [Release](#release---push-version-tag) process on the dev repo, the package will be published to https://www.npmjs.com/package/@appoptics/apm-bindings-dev. It should be [unpublished](https://docs.npmjs.com/unpublishing-packages-from-the-registry) as soon as possible. Note that because the package is scoped to the organization, the organization admin must temporarily reassign this package to just the dev-internal team; this team has a single member, which is one of the requirements for unpublishing per https://docs.npmjs.com/policies/unpublish#packages-published-more-than-72-hours-ago.

# Development & Release with GitHub Actions 

> **tl;dr** Push to feature branch. Create Pull Request. Merge Pull Request. Push version tag to release. 
> Package is always released in conjunction with AppOptics APM Agent. See [release proccess](https://github.com/appoptics/appoptics-apm-node/blob/master/docs/release-process.md) for details.

## Overview

The package is `node-pre-gyp` enabled and is published in a two step process. First prebuilt add-on tarballs are uploaded to an S3 bucket, and then an NPM package is published to the NPM . Prebuilt tarballs must be versioned with the same version as the NPM package and they must be present in the S3 bucket prior to the NPM package itself being published to the registry.

There are many platforms that can use the prebuilt add-on but will fail to build it, hence the importance of the prebuilts.

## Usage

### Prep - Push Dockerfile

* Push to master is disabled by branch protection.
* Push to branch which changes any Dockerfile in the `.github/docker-node/` directory will trigger [docker-node.yml](./workflows/docker-node.yml).
* Workflow will:
  - Build all Dockerfiles and create a [single package](https://github.com/appoptics/appoptics-bindings-node/pkgs/container/appoptics-bindings-node%2Fnode) named `node` scoped to `appoptics/appoptics-bindings-node` (the repo). Package has multiple tagged images for each of the dockerfiles from which it was built. For example, the image created from a file named `10-centos7-build.Dockerfile` has a `10-centos7-build` tag and can pulled from `ghcr.io/appoptics/appoptics-bindings-node/node:10-centos7-build`. Since this repo is public, the images are also public.
* Workflow creates (or recreates) images used in other workflows.
* Manual trigger supported.

```
push Dockerfile ─► ┌───────────────────┐ ─► ─► ─► ─► ─►
                   │Build Docker Images│ build & publish
manual ──────────► └───────────────────┘     
```

### Develop - Push

* Push to master is disabled by branch protection.
* Push to branch will trigger [push.yml](./workflows/push.yml). 
* Workflow will:
  - Build the code pushed on a default image. (`node` image from docker hub).
  - Run the tests against the build.
* Workflow confirms code is not "broken".
* Manual trigger supported. Enables to select node version.
* Naming a branch with `-no-action` ending disables this workflow. Use for documentation branches edited via GitHub UI.
```
push to branch ──► ┌───────────────────┐ ─► ─► ─► ─► ─►
                   │Single Build & Test│ contained build
manual (image?) ─► └───────────────────┘ ◄── ◄── ◄── ◄──
```

### Review - Pull Request

* Creating a pull request will trigger [review.yml](./workflows/review.yml). 
* Workflow will:
  - Build the code pushed on each of the Build Group images. 
  - Run the tests on each build.
* Workflow confirms code can be built in each of the required variations.
* Manual trigger supported. 
```
pull request ────► ┌──────────────────┐ ─► ─► ─► ─► ─►
                   │Group Build & Test│ contained build
manual ──────────► └──────────────────┘ ◄── ◄── ◄── ◄──
```
### Accept - Merge Pull Request 

* Merging a pull request will trigger [accept.yml](./workflows/accept.yml). 
* Workflow will:
  - Clear the *staging* S3* bucket of prebuilt tarballs (if exist for version).
  - Create all Fallback Group images and install. Since prebuilt tarball has been cleared, install will fallback to build from source.
  - Build the code pushed on each of the Build Group images.
  - Package the built code and upload a tarball to the *staging* S3 bucket. 
  - Create all Prebuilt Group images and install the prebuilt tarball on each.
* Workflow ensures node-pre-gyp setup (config and S3 buckets) is working for a wide variety of potential customer configurations.
* Manual trigger supported. Enables to select running the tests after install (on both Fallback & Prebuilt groups)

```
merge to master ─► ┌──────────────────────┐
                   │Fallback Group Install│
manual (test?) ──► └┬─────────────────────┘
                    │
                    │   ┌───────────────────────────┐ ─► ─► ─►
                    └─► │Build Group Build & Package│ S3 Package
                        └┬──────────────────────────┘ Staging
                         │
                         │   ┌──────────────────────┐     │
                         └─► │Prebuilt Group Install│ ◄── ▼
                             └──────────────────────┘
```

### Release - Push Version Tag

* Release process is `npm` and `git` triggered.
* To Release:
  1. On branch run `npm version {major/minor/patch}`(e.g. `npm version patch`) then have the branch pass through the Push/Pull/Merge flow above. 
  2. When ready `git push` origin {tag name} (e.g. `git push origin v11.2.3`).
* Pushing a semantic versioning tag for a patch/minor/major versions (e.g. `v11.2.3`) or an prerelease tagged pre-release (e.g. `v11.2.3-prerelease.2`) will trigger [release.yml](./workflows/release.yml). Pushing other pre-release tags (e.g. `v11.2.3-7`) is ignored.
* Workflow will: 
  - Build the code pushed in each of the Build Group images. 
  - Package the built code and upload a tarball to the *production* S3 bucket. 
  - Create all Target Group images and install the prebuilt tarball on each.
  - Publish an NPM package upon successful completion of all steps above. When version tag is `prerelease`, package will be NPM tagged same. When it is a release version, package will be NPM tagged `latest`.
* Workflow ensures node-pre-gyp setup is working in *production* for a wide variety of potential customer configurations.
* Workflow publishing to NPM registry exposes the NPM package (and the prebuilt tarballs in the *production* S3 bucket) to the public.
* Note: @appoptics/apm-bindings is not meant to be directly consumed. It is developed as a dependency of [appoptics-apm](https://www.npmjs.com/package/appoptics-apm).

```
push semver tag ─►  ┌────────────────────────────┐ ─► ─► ─►
push prerelease tag │Build Group Build & Package │ S3 Package
                    └┬───────────────────────────┘ Production
                     │
                     │   ┌────────────────────┐     │
                     └─► │Target Group Install│ ◄── ▼
                         └┬───────────────────┘
                          │
                          │   ┌───────────┐
                          └─► │NPM Publish│
                              └───────────┘
```
## Maintenance

> **tl;dr** There is no need to modify workflows. All data used is externalized.

### Definitions
* Local images are defined in [docker-node](./docker-node).
* S3 Staging bucket is defined in [package.json](../package.json).
* S3 Production bucket is defined in [package.json](../package.json).
* [Build Group](./config/build-group.json) are images on which the various versions of the add-on are built. They include combinations to support different Node versions and libc implementations. Generally build is done with the lowest versions of the OSes supported, so that `glibc`/`musl` versions are the oldest/most compatible.
* [Fallback Group](./config/fallback-group.json) images include OS and Node version combinations that *can* build for source.
* [Prebuilt Group](./config/prebuilt-group.json) images include OS and Node version combinations that *can not* build for source and thus require a prebuilt tarball.
* [Target Group](./config/target-group.json) images include a wide variety of OS and Node version combinations. Group includes both images that can build from code as well as those which can not.

### Adding an image to GitHub Container Registry

1. Create a docker file with a unique name to be used as a tag. Common is to use: `{node-version}-{os-name-version}` (e.g `16-ubuntu20.04.2.Dockerfile`). If image is a build image suffix with `-build`.
2. Place a Docker file in the `docker-node` directory.
3. Push to GitHub.

### Modifying group lists

1. Find available tags at [Docker Hub](https://hub.docker.com/_/node) or use path of image published to GitHub Container Registry (e.g. `ghcr.io/$GITHUB_REPOSITORY/node:14-centos7`)
2. Add to appropriate group json file in `config`.

### Adding a Node Version

1. Create an `alpine` builder image and a `centos` builder image. Use previous node version Dockerfiles as guide.
2. Create `alpine`, `centos` and `amazonlinux2` test images. Use previous node version Dockerfiles as guide.
3. Follow "Adding an image to GitHub Container Registry" above.
4. Follow "Modifying group lists" above.

### Remove a node version

1. Remove version images from appropriate group json file in `config`.
2. Leave `docker-node` Dockerfiles for future reference.

## Implementation

> **tl;dr** No Actions used. Matrix and Container directive used throughout.

### Workflows

1. All workflows `runs-on: ubuntu-latest`.
2. For maintainability and security custom actions are avoided.
3. Configuration has been externalized. All images groups are loaded from external json files located in the `config` directory.
4. Loading uses [fromJSON function and a standard two-job setup](https://docs.github.com/en/actions/reference/context-and-expression-syntax-for-github-actions#fromjson).
5. Loading is encapsulated in a [shell script](./scripts/matrix-from-json.sh). Since the script is not a "formal" action it is placed in a `script` directory.
3. All job steps are named.
5. Jobs are linked using `needs:`.

### Secrets

Repo is defined with the following secrets:
```
APPOPTICS_SERVICE_KEY
APPOPTICS_COLLECTOR
AO_TEST_PROD_SERVICE_KEY
SW_APM_SERVICE_KEY
SW_APM_COLLECTOR
SW_TEST_PROD_SERVICE_KEY

STAGING_AWS_ACCESS_KEY_ID
STAGING_AWS_SECRET_ACCESS_KEY
PROD_AWS_ACCESS_KEY_ID
PROD_AWS_SECRET_ACCESS_KEY
NPM_AUTH_TOKEN
```
# License

Copyright (c) 2016 - 2022 SolarWinds, LLC

Released under the [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0)

Fabriqué au Canada : Made in Canada 🇨🇦
