const expect = require('chai').expect

// change the environment so oboe will use udp
process.env.APPOPTICS_REPORTER = 'udp'
process.env.REPORTER_UDP = 'localhost:7832'

let bindings

describe('addon.context-udp', function () {
  before(function (done) {
    bindings = require('../..')
    bindings.oboeInit(process.env.APPOPTICS_SERVICE_KEY, {})
    bindings.Reporter.isReadyToSample(2000)
    done()
  })

  after(function () {

  })

  it('should tell us that a non-traced xtrace doesn\'t need to be sampled', function () {
    var md = bindings.Metadata.makeRandom(0)
    var settings = bindings.Context.getTraceSettings({xtrace: md.toString()})
    debugger
    expect(settings).property('status', -1)                     // -1 means non-sampled xtrace
    expect(settings.source).equal(2)                            // 2 compiled default
    expect(settings.metadata.toString().slice(-2)).equal('00')
  })

  it('should get verification that a request should be sampled', function (done) {
    bindings.Context.setTracingMode(bindings.TRACE_ALWAYS)
    bindings.Context.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE)
    var event = bindings.Context.createEventX(bindings.Metadata.makeRandom(1))
    var metadata = event.getMetadata()
    metadata.setSampleFlagTo(1)
    var xid = metadata.toString();
    var counter = 20
    // poll to give time for the SSL connection to complete. it should have
    // been waited on in before() but it's possible for the connection to break.
    var id = setInterval(function() {
      var settings = bindings.Context.getTraceSettings({xtrace: xid})
      if (settings.status == 0 && settings.source !== 2) {
        clearInterval(id)
        expect(settings).property('doSample', true)
        expect(settings).property('doMetrics', true)
        expect(settings).property('edge', true)
        expect(settings.metadata).instanceof(bindings.Metadata)
        expect(settings).property('source', 1)
        expect(settings).property('rate', bindings.MAX_SAMPLE_RATE)
        expect(settings).property('status', 0)
        done()
      } else {
        if (--counter <= 0) {
          done(new Error('getTraceSettings() never returned valid settings'))
        }
      }
    }, 50)
  })

})
