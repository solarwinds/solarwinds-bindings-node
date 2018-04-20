var bindings = require('../')

describe('addon.reporter', function () {
  var r = new bindings.Reporter()

  it('should initialize oboe with hostnameAlias', function () {
    bindings.oboeInit(process.env.APPOPTICS_SERVICE_KEY, {
      hostnameAlias: 'node-testing-hostname'
    })
  })

  it('should initialize oboe with empty options', function () {
    bindings.oboeInit(process.env.APPOPTICS_SERVICE_KEY, {})
  })

  it('should not crash node getting the prototype of a reporter instance', function () {
    var p = Object.getPrototypeOf(r)
  })
})
