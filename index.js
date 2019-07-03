if (process.env.NODE_ENV !== 'production') {
  try {
    segvHandler = require('segfault-handler');
    segvHandler.registerHandler('segv.log');
  } catch (e) {};
}
module.exports = require('bindings')('appoptics-bindings.node')
module.exports.version = require('./package.json').version
