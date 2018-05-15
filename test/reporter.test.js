var bindings = require('../')

var real = process.env.APPOPTICS_REPORTER === 'ssl'
  && (process.env.APPOPTICS_COLLECTOR === 'collector.appoptics.com'
  || process.env.APPOPTICS_COLLECTOR === 'collector-stg.appoptics.com')

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

  it('should check if ready to sample', function () {
    // wait 5 seconds max. This will fail if not using
    // a real SSL reporter (collector or collector-stg).
    var ready = r.isReadyToSample(5000)
    ready.should.be.an.instanceof(Number)

    if (real) {
      ready.should.equal(1)
    } else {
      ready.should.not.equal(1)
    }
  })

  it('should not crash node getting the prototype of a reporter instance', function () {
    var p = Object.getPrototypeOf(r)
  })
})
