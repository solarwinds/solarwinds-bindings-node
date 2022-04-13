#!/bin/bash

# for the name and version specified in package.json
pkg=$(node -e "console.log(require('./package.json').name)")
version=$(node -e "console.log(require('./package.json').version)")
echo "Checking $version of $pkg."

# check if package version was already published
published=$(npm view "$pkg" versions --json | grep "$version")

# if it was published - an error will halt workflow
if [ "$published" != "" ]; then
    echo "$published already published to NPM registry."
    exit 1 
fi
