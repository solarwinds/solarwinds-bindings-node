var bindings = require('../')

describe('addon.metadata', function () {
  var metadata
  var string

  it('should initialize oboe without options', function () {
    bindings.oboeInit(process.env.APPOPTICS_SERVICE_KEY)
  })

  it('should construct', function () {
    metadata = new bindings.Metadata()
  })

  it('should construct with random data', function () {
    metadata = bindings.Metadata.makeRandom()
    metadata.toString().should.not.equal('')
    metadata.toString().should.match(/2B[A-F0-9]{56}00/)
    metadata.getSampleFlag().should.equal(false)
  })

  it('should construct with random data with sample flag', function () {
    metadata = bindings.Metadata.makeRandom(1)
    metadata.toString().should.not.equal('')
    metadata.toString().should.match(/2B[A-F0-9]{56}01/)
    metadata.getSampleFlag().should.equal(true)
  })

  it('should set the sample flag', function () {
    var md = metadata.toString()
    metadata.setSampleFlagTo(1)
    // it shouldn't modify anything but the sample flag
    metadata.toString().slice(0, -2).should.equal(md.slice(0, -2))
    metadata.toString().slice(-2).should.equal('01')
    metadata.getSampleFlag().should.equal(true)
  })

  it('should clear the sample flag', function () {
    var md = metadata.toString()
    metadata.setSampleFlagTo(0)
    metadata.toString().slice(0, -2).should.equal(md.slice(0, -2))
    metadata.toString().slice(-2).should.equal('00')
    metadata.getSampleFlag().should.equal(false)
  })

  it('should set, clear, and return previous sample flag', function () {
    var md = metadata.toString()
    var onEntry = metadata.getSampleFlag()
    var previous

    previous = metadata.setSampleFlagTo(0)
    metadata.toString().slice(0, -2).should.equal(md.slice(0, -2))
    metadata.toString().slice(-2).should.equal('00')
    previous.should.equal(onEntry)

    previous = metadata.setSampleFlagTo(1)
    metadata.toString().slice(0, -2).should.equal(md.slice(0, -2))
    metadata.toString().slice(-2).should.equal('01')
    previous.should.equal(false)

    previous = metadata.setSampleFlagTo(onEntry)
    previous.should.equal(true)
  })

  it('should construct using metadata', function () {
    var md = new bindings.Metadata(metadata)
    md.toString().should.equal(metadata.toString())
  })

  it('should construct using an event', function () {
    var e = new bindings.Event(metadata)
    var md = new bindings.Metadata(e)
    md.toString().should.equal(e.toString())
  })

  it('should serialize to string', function () {
    string = metadata.toString()
    string.should.not.equal('')
  })

  it('should construct from string', function () {
    metadata = bindings.Metadata.fromString(string)
    metadata.toString().should.equal(string)
  })

  it('should not construct from invalid metadata', function () {
    var results = {}
    var xtrace = string.slice()
    var md = bindings.Metadata.fromString('0' + xtrace)
    results['xtrace too long'] = md
    md = bindings.Metadata.fromString('1' + xtrace.slice(1))
    results['invalid version'] = md
    md = bindings.Metadata.fromString('X' + xtrace.slice(1))
    results['invalid character'] = md
    md = bindings.Metadata.fromString('2b' + xtrace.slice(2))
    results['lowercase hex'] = md

    Object.keys(results).forEach(function (k) {
      var correct = results[k] === undefined
      correct.should.equal(true, k)
    })

  })

  it('should correctly test for the sample flag', function () {
    var mdNoSample = bindings.Metadata.makeRandom()
    var mdSample = bindings.Metadata.makeRandom(1)
    bindings.Metadata.sampleFlagIsSet(mdNoSample).should.equal(false)
    bindings.Metadata.sampleFlagIsSet(mdSample).should.equal(true)
    var e = new bindings.Event(mdNoSample)
    bindings.Metadata.sampleFlagIsSet(e).should.equal(false)
    bindings.Metadata.sampleFlagIsSet(e.toString()).should.equal(false)
    e = new bindings.Event(mdSample)
    bindings.Metadata.sampleFlagIsSet(e).should.equal(true)
    bindings.Metadata.sampleFlagIsSet(e.toString()).should.equal(true)

    // if there is an error it should return undefined.
    var result = typeof bindings.Metadata.sampleFlagIsSet('')
    result.should.equal('undefined')
  })

  it('should clone itself', function () {
    var rand = bindings.Metadata.makeRandom()
    rand.copy().toString().should.equal(rand.toString())
  })

  it('should be valid', function () {
    metadata.isValid().should.equal(true)
  })

  it('should create an event', function () {
    var event = metadata.createEvent()
    event.should.be.an.instanceof(bindings.Event)
  })

  it('should not crash node when getting the prototype of an metadata instance', function () {
    var p = Object.getPrototypeOf(bindings.Metadata.makeRandom())
  })
})
