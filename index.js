'use strict'

const binary = require('@mapbox/node-pre-gyp')
const path = require('path')
const pkgpath = path.resolve(path.join(__dirname, './package.json'))
const binding_path = binary.find(pkgpath) // eslint-disable-line camelcase
const bindings = require(binding_path)

module.exports = bindings

module.exports.version = require('./package.json').version
module.exports.init = function (sk) {
  return module.exports.oboeInit({ serviceKey: sk || process.env.APPOPTICS_SERVICE_KEY })
}

try {
  // this is hardcoded as node-pre-gyp doesn't know about multiple targets.
  const metrics_path = binding_path.replace('apm_bindings.node', 'ao_metrics.node') // eslint-disable-line camelcase
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

const validXtrace = (xtrace) => {
  // https://github.com/librato/trace/tree/master/docs/specs
  const regExp = /\b2B[0-9A-F]{40}[0-9A-F]{16}0[0-1]{1}\b/
  const matches = regExp.exec(xtrace)

  return matches ? matches[0] : ''
}

const validTraceparent = (traceparent) => {
  // https://www.w3.org/TR/trace-context/
  const regExp = /\b00-[0-9a-f]{32}-[0-9a-f]{16}-0[0-1]{1}\b/
  const matches = regExp.exec(traceparent)

  return matches
}

Event.makeFromString = function (string) {
  if (validXtrace(string)) return Event.makeFromBuffer(Buffer.from(string, 'hex'))
  if (validTraceparent(string)) return Event.makeFromBuffer(Buffer.from(string.replace(/-/g, ''), 'hex'))

  return undefined
}
