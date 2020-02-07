'use strict';

if (process.env.NODE_ENV !== 'production') {
  try {
    const segvHandler = require('segfault-handler');
    segvHandler.registerHandler('ao-segv.log');
  } catch (e) {}
}
module.exports = require('bindings')('appoptics-bindings.node')
module.exports.version = require('./package.json').version
module.exports.init = function (sk) {
  return module.exports.oboeInit({serviceKey: sk || process.env.APPOPTICS_SERVICE_KEY});
}

try {
  module.exports.metrics = require('bindings')('ao-metrics');
} catch (e) {}


