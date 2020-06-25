#!/bin/bash

#
# run tests that can be run with one initialization of bindings.
#
mocha test/*.test.js

#
# run tests that requires new invocations of bindings
#
mocha test/solo-tests/notifier.test.js
