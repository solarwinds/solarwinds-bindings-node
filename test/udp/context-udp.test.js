const expect = require('chai').expect

let bindings

describe('addon.context-udp', function () {
  before(function (done) {
    const initOptions = {
      serviceKey: process.env.APPOPTICS_SERVICE_KEY,
      reporter: 'udp',
      host: 'localhost:7832',
    }
    bindings = require('../..')
    bindings.oboeInit(initOptions);
    bindings.Reporter.isReadyToSample(2000)
    done()
  })

  after(function () {

  })

  it('should tell us that a non-traced xtrace doesn\'t need to be sampled', function () {
    var md = bindings.Metadata.makeRandom(0)
    var settings = bindings.Settings.getTraceSettings({xtrace: md.toString()})
    debugger
    expect(settings).property('status', -1)                     // -1 means non-sampled xtrace
    expect(settings.source).equal(6)                            // 6 remote default
    expect(settings.metadata.toString().slice(-2)).equal('00')
  })

  it('should get verification that a request should be sampled', function (done) {
    bindings.Settings.setTracingMode(bindings.TRACE_ALWAYS)
    bindings.Settings.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE)
    const event = new bindings.Event(bindings.Metadata.makeRandom(1));
    const metadata = event.getMetadata();
    metadata.setSampleFlagTo(1)
    var xid = metadata.toString();
    var counter = 20
    // poll to give time for the SSL connection to complete. it should have
    // been waited on in before() but it's possible for the connection to break.
    var id = setInterval(function() {
      var settings = bindings.Settings.getTraceSettings({xtrace: xid})
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
