const bindings = require('..')
const expect = require('chai').expect

const serviceKey = `${process.env.AO_TOKEN_PROD}:node-bindings-test`;

describe('addon.settings', function () {

  it('should initialize oboe with only a service key', function () {
    const result = bindings.oboeInit({serviceKey});
    // either already init'd or success.
    expect(result).oneOf([-1, 0]);
    bindings.Reporter.isReadyToSample(2000)
  })

  it('should set tracing mode to never', function () {
    bindings.Settings.setTracingMode(bindings.TRACE_NEVER)
  })
  it('should set tracing mode to always', function () {
    bindings.Settings.setTracingMode(bindings.TRACE_ALWAYS)
  })

  it('should throw setting invalid tracing mode value', function () {
    try {
      bindings.Settings.setTracingMode(3)
    } catch (e) {
      if (e.message === 'Invalid tracing mode') {
        return
      }
    }

    throw new Error('setTracingMode should fail on invalid inputs')
  })

  it('should throw setting invalid tracing mode type', function () {
    try {
      bindings.Settings.setTracingMode('foo')
    } catch (e) {
      if (e.message === 'Invalid arguments') {
        return
      }
    }

    throw new Error('setTracingMode should fail on invalid inputs')
  })

  it('should set valid sample rate', function () {
    bindings.Settings.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE / 10)
  })

  it('should not throw when setting invalid sample rate', function () {
    var threw = false
    try {
      bindings.Settings.setDefaultSampleRate(-1)
      bindings.Settings.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE + 1)
    } catch (e) {
      threw = true
    }

    expect(threw).equal(false, 'setting invalid sample rate must throw')
  })

  it('should handle bad sample rates correctly', function () {
    var rateUsed
    rateUsed = bindings.Settings.setDefaultSampleRate(-1)
    expect(rateUsed).equal(0)
    rateUsed = bindings.Settings.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE + 1)
    expect(rateUsed).equal(bindings.MAX_SAMPLE_RATE)

    rateUsed = bindings.Settings.setDefaultSampleRate(100000)
    expect(rateUsed).equal(100000)
    // the C++ code cannot ask oboe what rate was in effect. NaN doesn't not
    // change the value because it cannot be compared, so the addon returns -1.
    // appoptics-apm keeps a local copy of the value and handles this correctly.
    rateUsed = bindings.Settings.setDefaultSampleRate(NaN)
    expect(rateUsed).equal(-1)
  })

  it('should tell us that a non-traced xtrace doesn\'t need to be sampled', function () {
    var md = bindings.Metadata.makeRandom(0)
    var settings = bindings.Settings.getTraceSettings({xtrace: md.toString()})

    expect(settings).property('status', -1)       // -1 means non-sampled xtrace
  })

  it('should get verification that a request should be sampled', function (done) {
    bindings.Settings.setTracingMode(bindings.TRACE_ALWAYS)
    bindings.Settings.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE)
    var event = new bindings.Event(bindings.Metadata.makeRandom(1));
    var metadata = event.getMetadata()
    metadata.setSampleFlagTo(1)
    var xid = metadata.toString();
    var counter = 20
    // poll to give time for the SSL connection to complete. it should have
    // been waited on in before() but it's possible for the connection to break.
    var id = setInterval(function() {
      var settings = bindings.Settings.getTraceSettings({xtrace: xid})
      if (--counter <= 0 || typeof settings === 'object' && settings.source !== 2) {
        clearInterval(id)
        expect(settings).property('status', 0);
        expect(settings).property('doSample', true)
        expect(settings).property('doMetrics', true)
        expect(settings).property('edge', true)
        expect(settings.metadata).instanceof(bindings.Metadata)
        expect(settings).property('rate', bindings.MAX_SAMPLE_RATE);
        // the following depends on whether this suite is run standalone or with other
        // test files.
        expect(settings.source).oneOf([1, 6]);

        if (counter < 0) {
          done(new Error('getTraceSettings() never returned valid settings'))
        } else {
          done()
        }
      }
    }, 50)
  })

  it('should not set sample bit unless specified', function () {
    const md0 = bindings.Metadata.makeRandom(0)
    const md1 = bindings.Metadata.makeRandom(1)

    let event = new bindings.Event(md0)
    expect(event.getSampleFlag()).equal(false)
    expect(event.toString().slice(-2)).equal('00')

    event = new bindings.Event(md1)
    expect(event.getSampleFlag()).equal(true)
    expect(event.toString().slice(-2)).equal('01')
  })
})
