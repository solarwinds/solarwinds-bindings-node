# solarwinds-apm-bindings

[solarwinds-apm-bindings](https://github.com/solarwindscloud/solarwinds-bindings-node) is an NPM package containing a binary node add-on.

The package is installed as a dependency when the SolarWinds APM Agent ([solarwinds-apm](https://www.npmjs.com/package/solarwinds-apm)) is installed. This package itself only contains the logic for loading one of the platform-specific packages listed in `optionalDependencies`, one of which will be installed if the current platform is supported. When working locally, the local binary builds will be loaded by the package instead of the ones published on npmjs.

This library **only supports Linux**, on all maintained LTS versions of Node (currently 14, 16, 18), for both `glibc` 2.27+ (Ubuntu 18.04+, RHEL 8+, Debian 10+) and `musl` (Alpine, NixOS) based distributions.

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
- [Development & Release with GitHub Actions](#development--release-with-github-actions)
  * [Overview](#overview)
  * [Usage](#usage)
  * [Maintenance](#maintenance)
  * [Implementation](#implementation)
- [License](#license)

# Two minutes on how it works

The package implements a low-level interface to `liboboe`, a closed-source library maintained by SolarWinds. `liboboe` implements communications and aggregation functions to enable efficient sampling of traces. Traces are sequences of entry and exit events which capture performance information. 


# Local Development

Development used to be Linux-only, but is now supported on any platforms. However, Docker is still required to run tests as the library will only run on Linux.

## Project layout

`git clone` to start.

* `src` directory contains the C++ code to bind to liboboe. 
* `oboe` directory contains `liboboe` and its required header files. `liboboe` is downloaded from: https://agent-binaries.cloud.solarwinds.com/apm/c-lib. Pre-release versions are at: https://agent-binaries.global.st-ssp.solarwinds.com/apm/c-lib
* `test` directory contains the test suite. 
* `.github` contains the files for github actions.
* `dev` directory contains anything related to dev environment

## Docker Dev Container

1. Start the Docker daemon (on a Mac that would be simplest using Docker desktop).
2. Create a `.env` file and set keys for the backend:
- `SW_TEST_PROD_SERVICE_KEY={a valid **production** service key}`
- `SW_APM_SERVICE_KEY={a valid service key to any of dev/staging/production}`
- `SW_APM_COLLECTOR={optional url of the collector at dev/staging}`
3. Run `npm run dev`. This will create a docker container, set it up, and open a shell. Repo code is **mounted** to the container.
4. To open another shell in same container use: `docker exec -it dev-bindings /bin/bash`

The setup script ensures a "clean" work place with each run by removing artifacts and installed modules on each exit.

## Repo Packages

This repo has a "single" GitHub package named `node` scoped to `solarwindscloud/solarwinds-bindings-node` (the repo) which has [multiple tagged images](https://github.com/solarwindscloud/solarwinds-bindings-node/pkgs/container/solarwinds-bindings-node%2Fnode).

Those images complement the official node (https://hub.docker.com/_/node) and RedHat (https://catalog.redhat.com/software/containers/search?q=nodejs) images with specific end-user configurations.

## "One Off" Docker Container

At times it may be useful to set a "one off" docker container to test a specific feature or build.

1. Run `npm run dev:oneoff`. This will create a docker container, set it up, and open a shell. Repo code is **copied** to the container.
2. To specify an image to the "one off" container pass it as argument. For example: run `npm run dev:oneoff node:latest` to get latest official image or `npm run dev:oneoff ghcr.io/solarwindscloud/solarwinds-bindings-node/node:14-alpine3.12` to get one of this repo custom images.

## Testing

Test are run using [Mocha](https://github.com/mochajs/mocha).

1. Run `npm test` to run the test suite against the collector specified in the `.env` file (`SW_APM_COLLECTOR`).

Note: the initial default initialization test will always run against production collector using `SW_TEST_PROD_SERVICE_KEY` from the .env file.

The `test` script in `package.json` runs `test.sh` which then manages how mocha runs each test file. To run individual tests use `npx mocha`. For example: `npx mocha test/config.test.js` will run the config tests.

## Building

Building is done using [zig-build](https://github.com/solarwindscloud/zig-build).

The build script can be found at [`build.js`](../build.js). It will build for all supported targets for the current Node version. To build for all supported versions, use `npm run build:all` which will run the build script in a container for each version.

## Debugging

Debugging node addons is not intuitive but this might help (from [stackoverflow](https://stackoverflow.com/questions/23228868/how-to-debug-binary-module-of-nodejs))

First, change the build script and add `mode: 'debug'` to the target you wish to debug.

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


# Development & Release with GitHub Actions 

> **tl;dr** Push to feature branch. Create Pull Request. Merge Pull Request. Manual release. 
> Package is always released in conjunction with SolarWinds APM Agent. See [release process](https://github.com/solarwindscloud/solarwinds-apm-node/blob/main/docs/release-process.md) for details.


## Overview

The package is published in a two step process. First, the platform-specific packages generated in the `npm` directory by the build script are published to NPM. The tests are then ran using the published prebuilt packages to make sure they work before finally publishing the `solarwinds-apm-bindings` package itself.

The version should be bumped prior to starting the release workflow using `npm version`, as it will run a script that takes care of syncing the platform-specific dependency versions, which otherwise has to be done manually too. 

The library previously used `node-pre-gyp`, allowing the dependents to build it themselves if no prebuilt package was available for their platform. However this is no longer the case, as we already publish prebuilt packages for every supported platform, this change simply makes this policy more explicit.

## Usage

### Prep - Push Dockerfile

* Push to main is disabled by branch protection.
* Push to branch which changes any Dockerfile in the `.github/docker-node/` directory will trigger [docker-node.yml](./workflows/docker-node.yml).
* Workflow will:
  - Build all Dockerfiles and create a [single package](https://github.com/solarwindscloud/solarwinds-bindings-node/pkgs/container/solarwinds-bindings-node%2Fnode) named `node` scoped to `solarwindscloud/solarwinds-bindings-node` (the repo). Package has multiple tagged images for each of the dockerfiles from which it was built. For example, the image created from a file named `18-amazonlinux2022.Dockerfile` has a `18-amazonlinux2022` tag and can pulled from `ghcr.io/solarwindscloud/solarwinds-bindings-node/node:18-amazonlinux2022`. Since this repo is public, the images are also public.
* Workflow creates (or recreates) images used in other workflows.
* Manual trigger supported.

```
push Dockerfile ─► ┌───────────────────┐ ─► ─► ─► ─► ─►
                    │Build Docker Images│ build & publish
manual ──────────► └───────────────────┘     
```

### Develop - Push

* Push to main is disabled by branch protection.
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
  - Build the code for all supported platforms and Node versions.
  - Run the tests on each platform in the x64 group.
* Workflow confirms code can be built in each of the required variations.
* Manual trigger supported. 
```
pull request ────► ┌──────────────────┐ ─► ─► ─► ─► ─►
                    │Group Build & Test│ contained build
manual ──────────► └──────────────────┘ ◄── ◄── ◄── ◄──
```

### Release - Manual

* Release process is `npm` and GitHub Actions triggered.
* To Release:
  1. On branch run `npm version {major/minor/patch}`(e.g. `npm version patch`) then have the branch pass through the Push/Pull/Merge flow above. 
  2. When ready - manually trigger the Release workflow.
* Workflow will: 
  - Build the code for all supported platforms and Node versions.
  - Publish each platform-specific packages generated in the `npm` directory.
  - Run the tests on each platform in the x64 group using the published packages.
  - Publish the `solarwinds-apm-bindings` NPM package upon successful completion of all steps above. When version tag is `prerelease`, package will be NPM tagged same. When it is a release version, package will be NPM tagged `latest`.
* Workflow ensures `optionalDependencies` setup is working in *production* for a wide variety of potential customer configurations.
* Workflow publishing to NPM registry exposes the NPM package to the public.
* Note: solarwinds-apm-bindings is not meant to be directly consumed. It is developed as a dependency of [solarwinds-apm](https://www.npmjs.com/package/solarwinds-apm).

```               ┌───────────────────┐
manual ─────────►│Confirm Publishable│
                  └┬──────────────────┘
                   │
                   │  ┌──────────────────────────────────┐
                   └►│Platform-specific build & publish │ ─► npmjs
                      └┬─────────────────────────────────┘
                       │                                         │
                       │   ┌────────────────────┐                │
                       └─►│Group Install & Test│◄──────────────┘
                           └┬───────────────────┘
                            │
                            │   ┌───────────┐
                            └─►│NPM Publish│
                                └───────────┘

```

## Maintenance

> **tl;dr** There is no need to modify workflows. All data used is externalized.

### Definitions
* Local images are defined in [docker-node](./docker-node).
* [x64 Group](./config/x64.json) images include a wide variety of x64 (x86_64) OS and Node version combinations.

### Adding an image to GitHub Container Registry

1. Create a docker file with a unique name to be used as a tag. Common is to use: `{node-version}-{os-name-version}` (e.g `18-amazonlinux2022.Dockerfile`).
2. Add the entry to the [`docker-node.json`](./config/docker-node.json) file.
3. Push to GitHub.

### Modifying group lists

1. Find available tags at [Docker Hub](https://hub.docker.com/_/node) or [RedHat](https://catalog.redhat.com/software/containers/search?q=nodejs) or use path of image published to GitHub Container Registry (e.g. `ghcr.io/$GITHUB_REPOSITORY/node:18-amazonlinux2022`)
2. Add to appropriate group json file in `config`.

### Adding a Node Version

1. Create or find `alpine`, `amazonlinux2022`, `debian` (10+), `ubi` (RHEL 8+) and `ubuntu` (18.04+) images. Use previous node version Dockerfiles as guide.
2. Follow "Adding an image to GitHub Container Registry" above.
3. Follow "Modifying group lists" above.

### Remove a node version

1. Remove version images from appropriate group json file in `config`.
<!-- 2. Leave `docker-node` Dockerfiles for future reference. Not really necessary, git stores the entire repo history after all. -->

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

Repo is defined with the following secrets 
For testing:
```
SW_APM_COLLECTOR
SW_APM_SERVICE_KEY
SW_TEST_PROD_SERVICE_KEY
```
For Release:
```
NPM_AUTH_TOKEN
```

# License

Copyright (c) 2016 - 2022 SolarWinds, LLC

Released under the [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0)

Fabriqué au Canada : Made in Canada 🇨🇦
