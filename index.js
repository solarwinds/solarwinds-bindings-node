'use strict'

const binary = require('@mapbox/node-pre-gyp')
const path = require('path')
const pkgpath = path.resolve(path.join(__dirname, './package.json'))
const binding_path = binary.find(pkgpath)
const bindings = require(binding_path)

module.exports = bindings

module.exports.version = require('./package.json').version
module.exports.init = function (sk) {
  return module.exports.oboeInit({ serviceKey: sk || process.env.APPOPTICS_SERVICE_KEY })
}

try {
  // this is hardcoded as node-pre-gyp doesn't know about multiple targets.
  const metrics_path = binding_path.replace('apm_bindings.node', 'ao_metrics.node')
  module.exports.metrics = require(metrics_path)
} catch (e) {
  // eslint-disable-next-line no-console
  console.warn(`appoptics metrics disabled ${e.message}`)
  // return an dummy metrics if this can't be loaded
  module.exports.metrics = {
    start () { return true },
    stop () { return true },
    getMetrics () { return {} }
  }
}
const Event = module.exports.Event

//
// augment the c++ event object with a string conversion
// function.
//
Event.makeFromString = function (string) {
  if (!string || string.length != 60) {
    return undefined
  }
  const b = Buffer.from(string, 'hex')
  if (b.length !== 30 || b[0] !== 0x2b || b[29] & 0xFE) {
    return undefined
  }
  // an all zero op id is not valid.
  if (string.startsWith('0'.repeat(16), 42)) {
    return undefined
  }

  return Event.makeFromBuffer(b)
}
