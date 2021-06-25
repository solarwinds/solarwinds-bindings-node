# Develop & Release with GitHub Actions 

> **tl;dr** Push to feature branch. Create Pull Request. Merge Pull Request. Push version tag to release.

## Overview

[@appoptics/apm-bindings](https://www.npmjs.com/package/@appoptics/apm-bindings) is an NPM package containing a binary add-on. 

When installed, it will try to install a prebuilt add-on using [node-pre-gyp](https://github.com/mapbox/node-pre-gypand) and only if that fails, will it attempt to build the add-on from source using [node-gyp](https://github.com/nodejs/node-gyp). There are many platforms that can use the prebuilt add-on but will fail to build it, hence the importance of the prebuilts.

The package is published using node-pre-gyp in a two step process. First prebuilt add-on tarballs are uploaded to an S3 bucket, and then an NPM package is published to the NPM registry. prebuilt tarballs must be versioned with the same version as the NPM package and they must be present in the S3 bucket prior to the NPM package itself being published to the registry.

## Use

### Workflows and Events

#### Develop - Push

* Push to master is disabled by branch protection.
* Push to branch will trigger [push.yml](./workflows/push.yml). 
* Workflow will:
  - Build the code pushed on a default image. (`node` image from docker hub).
  - Run the tests against the build.
* Workflow confirms code is not "broken".
* Manual trigger supported. Enables to select image from docker hub or local files (e.g. `erbium-buster-slim`) to build code on.

#### Review - Pull Request

* Creating a pull request will trigger [review.yml](./workflows/review.yml). 
* Workflow will:
  - Build the code pushed on each of the Build Group images. 
  - Run the tests on each build.
* Workflow confirms code can be built.
* Manual trigger supported. 

#### Accept - Merge Pull Request 

* Merging the pull request will trigger [accept.yml](./workflows/accept.yml). 
* Workflow will run two paths.
* Path one will:
  - Try to install on the Fallback Group images. Since prebuilt tarball version is not yet available, they will fallback to build from source.
* Path two will:
  - Build the code pushed on each of the Build Group images.
  - Package the built code and upload a tarball to the *staging* S3 bucket. 
  - Create all Target Group images and install the prebuilt tarball on each.
* Workflow ensures node-pre-gyp setup is working for a wide variety of potential customer configurations.
* Manual trigger supported. Enables to select running the tests on each of the Target Group images.

#### Release - Push Version Tag

* Release process is `npm` and `git` triggered.
* To Release:
  1. On branch run `npm version {major/minor/patch}`(e.g. `npm version patch`) then have the branch pass through the Push/Pull/Merge flow above. 
  2. When ready `git push` origin {tag name} (e.g. `git push origin v11.2.3`).
* Pushing a semantic versioning tag for a patch/minor/major versions (e.g. `v11.2.3`) or an alpha tagged pre-release (e.g. `v11.2.3-alpha.2`) will trigger [release.yml](./workflows/release.yml). Pushing other tags (e.g. `v11.2.3-7`) is ignored.
* Workflow will: 
  - Build the code pushed in each of the Build Group images. 
  - Package the built code and upload a tarball to the *production* S3 buckets. 
  - Create all Target Group images and install the prebuilt tarball on each.
  - Publish an NPM package upon successful completion of all steps above. When version tag is `alpha`, package will be tagged same. When it is a release version, package will be tagged `latest`.
* Workflow ensures node-pre-gyp setup is working in *production* for a wide variety of potential customer configurations.
* Workflow publishing to NPM registry exposes the NPM package (and the prebuilt tarballs in the *production* S3 bucket) to the public.
* Note: @appoptics/apm-bindings is not meant to be directly consumed. It is developed as a dependency of [appoptics-apm](https://www.npmjs.com/package/appoptics-apm).

### Workflow Diagram

#### Test Workflows
```
push to branch ──► ┌───────────────────┐ ─► ─► ─► ─► ─►
                   │Single Build & Test│ contained build
manual (image?) ─► └───────────────────┘ ◄── ◄── ◄── ◄──


pull request ────► ┌──────────────────┐ ─► ─► ─► ─► ─►
                   │Group Build & Test│ contained build
manual ──────────► └──────────────────┘ ◄── ◄── ◄── ◄──


merge to master ┌► ┌───────────────────────────┐ ─► ─► ─►
                │  │Build Group Build & Package│ S3 Package
              ┌─┤► └┬──────────────────────────┘ Staging
              │ │   │
              │ │   │   ┌──────────────────────┐     │
              │ │   └─► │Prebuilt Group Install│ ◄── ▼
              │ │       └──────────────────────┘
              │ │
              │ └► ┌──────────────────────┐
              │    │Fallback Group Install│
manual (test?)└──► └──────────────────────┘
```
#### Release Workflow

```
push semver tag ─► ┌────────────────────────────┐ ─► ─► ─►
push alpha tag     │Build Group Build & Package │ S3 Package
                   └┬───────────────────────────┘ Production
                    │
                    │   ┌────────────────────┐     │
                    └─► │Target Group Install│ ◄── ▼
                        └┬───────────────────┘
                         │
                         ▼
                    ┌───────────┐
                    │NPM Publish│
                    └───────────┘
```

## Maintain

> **tl;dr** There is no need to modify workflows. All data used is externalized.

### Definitions
* Local images are defined in [docker-node](./docker-node) ordered by node version.
* S3 Staging bucket is defined in [package.json](../package.json).
* S3 Production bucket is defined in [package.json](../package.json).
* Build Group are images on which the various versions of the add-on are built. They include combinations to support different Node versions and libc implementations. Generally build is done with the lowest versions of the OSes supported, so that `glibc`/`musl` versions are the oldest/most compatible. They are listed in [config/build-group.json](./config/build-group.json) and defined using local Dockerfile.
* Fallback Group images are listed in [config/fallback-group.json](./config/fallback-group.json) and defined using [images from Docker Hub](https://hub.docker.com/_/node). They include OS and Node version combinations that *can* build for source.
* Prebuilt Group images are listed in [config/prebuilt-group.json](./config/prebuilt-group.json) and defined using [images from Docker Hub](https://hub.docker.com/_/node) or using local Dockerfiles. They include OS and Node version combinations that *can not* build for source and thus require a prebuilt tarball.
* Target Group images are listed in [config/target-group.json](./config/target-group.json) and defined using [images from Docker Hub](https://hub.docker.com/_/node) or using local Dockerfiles. They include a wide variety of OS and Node version combinations. Group includes both images that can build from code as well as those which can not.

### Adding a local Dockerfile

1. Find or Create a folder under `docker-node` named for the version of node the Dockerfile is created for (e.g. `16`).
2. Create a folder to represent the operating system. Any string is valid. Use something descriptive (e.g. `ubuntu20.04.2`).
3. Place a Docker file in the directory.

### Modifying group lists

1. Find available tags at: https://hub.docker.com/_/node or use local Dockerfile path (e.g. `./.github/docker-node/10/ubuntu20.04.2`)
2. Add to appropriate group json file in `config`.

### Adding a Node Version

1. Create a folder under `docker-node` named for the version of node added.
2. Create an `alpine` builder image and a `centos` builder image. Use previous node version Dockerfiles as guide.
3. Add to appropriate group json files in `config`.

### Remove a node version

1. Remove version images from appropriate group json file in `config`.
2. Leave the directory in `docker-node` for future reference.

## Implementation

> **tl;dr** No Actions used. Simple script gets image, returns running container id. Steps exec on container.

### Workflows

1. All workflows `runs-on: ubuntu-latest`.
2. Workflows only use `actions/checkout@v2`. No other actions are used. For maintainability and security custom actions are avoided and shell scripts preferred.
3. All jobs start with a two-step process:
  - check out code:
    ```yaml
     - name: Checkout ${{ github.ref }}
       uses: actions/checkout@v2
    ```
  - create a container loaded with repo code:
    ```yaml
      - name: Create Container  ${{ matrix.image }}
        id: create
        run: .github/scripts/container-from-matrix-input.sh ${{ matrix.image }}
    ```
4. All job steps are named.
5. All following steps are executed using `docker exec` against the created container.
6. All jobs end with stopping the container:
  ```yaml
      - name: Stop container
        if: always()
        run: docker stop ${{ steps.create.outputs.containerId }}
  ```
7. Job are linked using `needs:`.

### Secrets

1. Repo is defined with 6 secrets:
```
AO_TOKEN_PROD
AWS_ACCESS_KEY_ID
AWS_SECRET_ACCESS_KEY
PROD_AWS_ACCESS_KEY_ID
PROD_AWS_SECRET_ACCESS_KEY
NPM_AUTH_TOKEN
```
2. AO_TOKEN_PROD is set as environment variable in all jobs that create a container. It is picked up by the created container via ```container-from-matrix-input.sh```
```
    env:
      AO_TOKEN_PROD: ${{ secrets.AO_TOKEN_PROD }}
```
3. Credential secrets are explicitly passed when needed.

Fabriqué au Canada : Made in Canada 🇨🇦
