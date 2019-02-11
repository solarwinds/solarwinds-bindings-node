## appoptics-bindings-node release building

Using N-API the appoptics-bindings.node file is compiled and the resulting binary is distributed as part of the appoptics-bindings package.

The precompiled binary file that is distributed links to specific versions GLIBCXX in libc. Because of this installation will fail at runtime if the correct libraries are not present. For example, on my development machine gcc 7.3.0 will link to GLIBCXX 3.4.25 in libstdc++.so.6. If I then take the appoptics-bindings.node binary to a debian 8 or alpine machine it won't load. debian 8's version of libstdc++.so.6 only includes GLIBCXX 3.4.20.

The solution I've chosen is to link with the a low version of gcc so that most of our target environments have the correct support. Using debian 8 to compile results in a binary that passes our test suite on alpine 3.4.6+, debian 8+, and ubuntu 16+. It may well work on other environments but they are not tested.

### the release-build files

- release-tool.sh - this script is the only one that is run directly
- Dockerfile - used by release-tool.sh to build the image(s) for building appoptics-bindings
- install-software.sh - used by Dockerfile to install needed software
- building-bindings.sh - is run as the Dockerfile's CMD

### the release-build process

- `ACTION=build-image ./release-tool.sh` - this builds the image for the default build environment.
- `./release-tool.sh` (or `ACTION=build-bindings ./release-tool.sh`) - builds the bindings and places them in `$PWD/build/` under the OS image used (debian, alpine, ubuntu).


