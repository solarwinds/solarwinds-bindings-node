var bindings = require('../')

describe('addon.context', function () {
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
      if (e.message === 'Tracing mode must be a number') {
        return
      }
    }

    throw new Error('setTracingMode should fail on invalid inputs')
  })

  it('should set valid sample rate', function () {
    bindings.Context.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE / 10)
  })
  it('should throw setting invalid sample rate', function () {
    try {
      bindings.Context.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE + 1)
    } catch (e) {
      if (e.message === 'Sample rate out of range') {
        return
      }
    }

    throw new Error('setDefaultSampleRate should fail on invalid inputs')
  })

  it('should serialize context to string', function () {
    bindings.Context.clear()
    var string = bindings.Context.toString()
    string.should.equal('2B0000000000000000000000000000000000000000000000000000000000')
  })
  it('should set context to metadata instance', function () {
    var event = bindings.Context.createEvent()
    var metadata = event.getMetadata()
    bindings.Context.set(metadata)
    var v = bindings.Context.toString()
    v.should.not.equal('')
    v.should.equal(metadata.toString())
  })
  it('should set context to metadata instance using JavaScript', function () {
    var md = bindings.Metadata.fromContext()
    var event = new bindings.Event(md)
    bindings.Context.set(event.getMetadata())
    var v = bindings.Context.toString()
    v.should.not.equal('')
    v.should.equal(event.getMetadata().toString())
  })
  it('should set context from metadata string', function () {
    var event = bindings.Context.createEvent()
    var string = event.getMetadata().toString()
    bindings.Context.set(string)
    var v = bindings.Context.toString()
    v.should.not.equal('')
    v.should.equal(string)
  })
  it('should copy context to metadata instance', function () {
    var metadata = bindings.Context.copy()
    var v = bindings.Context.toString()
    v.should.not.equal('')
    v.should.equal(metadata.toString())
  })
  it('should clear the context', function () {
    var string = '2B0000000000000000000000000000000000000000000000000000000000'
    bindings.Context.toString().should.not.equal(string)
    bindings.Context.clear()
    bindings.Context.toString().should.equal(string)
  })

  it('should create an event from the current context', function () {
    var event = bindings.Context.createEvent()
    event.should.be.an.instanceof(bindings.Event)
  })
  it('should start a trace from the current context', function () {
    var event = bindings.Context.startTrace()
    event.should.be.an.instanceof(bindings.Event)
  })

  it('should get verification that a request should be sampled', function (done) {
    bindings.Context.setTracingMode(bindings.TRACE_ALWAYS)
    bindings.Context.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE)
    var event = bindings.Context.startTrace()
    var xid = event.getMetadata().toString().slice(0, -1) + '\u0001'
    var counter = 8
    // poll to give time for the SSL connection to complete
    var id = setInterval(function() {
      // AO sampleRequest() requires service-name and X-Trace ID to check if sampling.
      var check = bindings.Context.sampleTrace('bruce-test', xid)
      if (--counter <= 0 || typeof check === 'object') {
        clearInterval(id)
        check.should.have.property('sample', true)
        check.should.have.property('source', 1)
        check.should.have.property('rate', bindings.MAX_SAMPLE_RATE)
        done()
        return
      }
  }, 500)
  })

  it('should be invalid when empty', function () {
    bindings.Context.clear()
    bindings.Context.isValid().should.equal(false)
  })
  it('should be valid when not empty', function () {
    var event = bindings.Context.startTrace()
    bindings.Context.isValid().should.equal(true)
  })
})
