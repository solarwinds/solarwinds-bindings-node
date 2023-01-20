'use strict'

function platform () {
  const cpu = process.arch
  const os = process.platform
  const node = process.version.split('.')[0].slice(1)

  if (os === 'linux') {
    const glibc = process.report.getReport().header.glibcVersionRuntime
    const abi = glibc ? 'gnu' : 'musl'

    return `${cpu}-linux-${abi}-${node}`
  } else {
    return `${cpu}-${os}-${node}`
  }
}
const p = platform()

let bindings
let metrics

try {
  bindings = require(`solarwinds-apm-bindings-${p}/bindings.node`)
  metrics = require(`solarwinds-apm-bindings-${p}/metrics.node`)
} catch {
  try {
    bindings = require(`./npm/${p}/bindings.node`)
    metrics = require(`./npm/${p}/metrics.node`)

    module.exports = bindings

    module.exports.version = require('./package.json').version
    module.exports.init = function (sk) {
      return module.exports.oboeInit({ serviceKey: sk || process.env.SW_APM_SERVICE_KEY })
    }

    module.exports.metrics = metrics

    const Event = module.exports.Event

    //
    // augment the c++ event object with a string conversion
    // function.
    //

    const validTraceparent = (traceparent) => {
      // https://www.w3.org/TR/trace-context/
      const regExp = /^00-[0-9a-f]{32}-[0-9a-f]{16}-0[0-1]{1}$/
      const matches = regExp.exec(traceparent)

      return matches
    }

    Event.makeFromString = function (string) {
      if (validTraceparent(string)) return Event.makeFromBuffer(Buffer.from(string.replace(/-/g, ''), 'hex'))
    }
  } catch {
    console.warn(`platform ${p} not supported`)
  }
}
