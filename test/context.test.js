const bindings = require('../')
const expect = require('chai').expect

const serviceKey = `${process.env.AO_TOKEN_PROD}:node-bindings-test`;

describe('addon.context', function () {

  it('should initialize oboe with only a service key', function () {
    const result = bindings.oboeInit({serviceKey});
    expect(result).equal(0);
    bindings.Reporter.isReadyToSample(2000)
  })

  it('should set tracing mode to never', function () {
    bindings.Context.setTracingMode(bindings.TRACE_NEVER)
  })
  it('should set tracing mode to always', function () {
    bindings.Context.setTracingMode(bindings.TRACE_ALWAYS)
  })

  it('should throw setting invalid tracing mode value', function () {
    try {
      bindings.Context.setTracingMode(3)
    } catch (e) {
      if (e.message === 'Invalid tracing mode') {
        return
      }
    }

    throw new Error('setTracingMode should fail on invalid inputs')
  })

  it('should throw setting invalid tracing mode type', function () {
    try {
      bindings.Context.setTracingMode('foo')
    } catch (e) {
      if (e.message === 'Invalid arguments') {
        return
      }
    }

    throw new Error('setTracingMode should fail on invalid inputs')
  })

  it('should set valid sample rate', function () {
    bindings.Context.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE / 10)
  })

  it('should not throw when setting invalid sample rate', function () {
    var threw = false
    try {
      bindings.Context.setDefaultSampleRate(-1)
      bindings.Context.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE + 1)
    } catch (e) {
      threw = true
    }

    expect(threw).equal(false, 'setting invalid sample rate must throw')
  })

  it('should handle bad sample rates correctly', function () {
    var rateUsed
    rateUsed = bindings.Context.setDefaultSampleRate(-1)
    expect(rateUsed).equal(0)
    rateUsed = bindings.Context.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE + 1)
    expect(rateUsed).equal(bindings.MAX_SAMPLE_RATE)

    rateUsed = bindings.Context.setDefaultSampleRate(100000)
    expect(rateUsed).equal(100000)
    // the C++ code cannot ask oboe what rate was in effect. NaN doesn't not
    // change the value because it cannot be compared, so the addon returns -1.
    // appoptics-apm keeps a local copy of the value and handles this correctly.
    rateUsed = bindings.Context.setDefaultSampleRate(NaN)
    expect(rateUsed).equal(-1)
  })

  it('should serialize context to string', function () {
    bindings.Context.clear()
    var string = bindings.Context.toString()
    expect(string).equal('2B0000000000000000000000000000000000000000000000000000000000')
  })

  it('should set context to metadata instance', function () {
    var md = bindings.Metadata.fromContext()
    var event = new bindings.Event(md)
    bindings.Context.set(event.getMetadata())
    const v = bindings.Context.toString()
    expect(v).not.equal('')
    expect(v).equal(event.getMetadata().toString())
  })

  it('should set context from metadata string', function () {
    var md =  bindings.Metadata.fromContext()
    var event = new bindings.Event(md)
    var string = event.getMetadata().toString()
    bindings.Context.set(string)
    var v = bindings.Context.toString()
    expect(v).not.equal('')
    expect(v).equal(string)
  })

  it('should clear the context', function () {
    var string = '2B0000000000000000000000000000000000000000000000000000000000'
    expect(string).not.equal(bindings.Context.toString())
    bindings.Context.clear()
    expect(string).equal(bindings.Context.toString())
  })

  it('should create an event from the current context', function () {
    // Event should use current context if no argument is supplied.
    const event = new bindings.Event()
    expect(event).instanceOf(bindings.Event)
  })

  it('should start a trace from the current context', function () {
    var event = bindings.Context.createEventX()
    expect(event).instanceOf(bindings.Event)
  })

  it('should allow any signature of createEventX', function () {
    var string = '2B0000000000000000000000000000000000000000000000000000000000'
    var md = bindings.Metadata.makeRandom()
    var event = bindings.Context.createEventX()
    expect(event).instanceOf(bindings.Event)

    event = bindings.Context.createEventX(string)
    expect(event).instanceOf(bindings.Event)

    event = bindings.Context.createEventX(event)
    expect(event).instanceOf(bindings.Event)

    event = bindings.Context.createEventX(md)
    expect(event).instanceOf(bindings.Event)

    event = bindings.Context.createEventX(md, false)
    expect(event).instanceOf(bindings.Event)

    event = bindings.Context.createEventX(md, true)
    expect(event).instanceOf(bindings.Event)
  })

  it('should tell us that a non-traced xtrace doesn\'t need to be sampled', function () {
    var md = bindings.Metadata.makeRandom(0)
    var settings = bindings.Context.getTraceSettings({xtrace: md.toString()})

    expect(settings).property('status', -1)       // -1 means non-sampled xtrace
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
      if (--counter <= 0 || typeof settings === 'object' && settings.source !== 2) {
        clearInterval(id)
        expect(settings).property('doSample', true)
        expect(settings).property('doMetrics', true)
        expect(settings).property('edge', true)
        expect(settings.metadata).instanceof(bindings.Metadata)
        expect(settings).property('source', 6)
        expect(settings).property('rate', bindings.MAX_SAMPLE_RATE)
        expect(settings).property('status', 0)

        if (counter < 0) {
          done(new Error('getTraceSettings() never returned valid settings'))
        } else {
          done()
        }
      }
    }, 50)
  })

  it('should be invalid when empty', function () {
    bindings.Context.clear()
    expect(bindings.Context.isValid()).equal(false)
  })

  it('should be valid when not empty', function () {
    const md = bindings.Metadata.makeRandom(1)
    bindings.Context.set(md)
    expect(bindings.Context.isValid()).equal(true)
  })

  it('should not set sample bit unless specified', function () {
    const md0 = bindings.Metadata.makeRandom(0)
    const md1 = bindings.Metadata.makeRandom(1)

    let event = bindings.Context.createEventX(md0)
    expect(event.getSampleFlag()).equal(false)
    expect(event.toString().slice(-2)).equal('00')

    event = bindings.Context.createEventX(md1)
    expect(event.getSampleFlag()).equal(true)
    expect(event.toString().slice(-2)).equal('01')
  })
})
