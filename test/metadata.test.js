var bindings = require('../')
const expect = require('chai').expect

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

    expect(metadata.toString()).not.equal('')
    expect(metadata.toString()).match(/2B[A-F0-9]{56}00/)
    expect(metadata.getSampleFlag()).equal(false)
  })

  it('should construct with random data with sample flag', function () {
    metadata = bindings.Metadata.makeRandom(1)

    expect(metadata.toString()).not.equal('')
    expect(metadata.toString()).match(/2B[A-F0-9]{56}01/)
    expect(metadata.getSampleFlag()).equal(true)
  })

  it('should set the sample flag', function () {
    var md = metadata.toString()
    metadata.setSampleFlagTo(1)

    // it shouldn't modify anything but the sample flag
    expect(metadata.toString().slice(0, -2)).equal(md.slice(0, -2))
    expect(metadata.toString().slice(-2)).equal('01')
    expect(metadata.getSampleFlag()).equal(true)
  })

  it('should clear the sample flag', function () {
    var md = metadata.toString()
    metadata.setSampleFlagTo(0)

    expect(metadata.toString().slice(0, -2)).equal(md.slice(0, -2))
    expect(metadata.toString().slice(-2)).equal('00')
    expect(metadata.getSampleFlag()).equal(false)
  })

  it('should set, clear, and return previous sample flag', function () {
    var md = metadata.toString()
    var onEntry = metadata.getSampleFlag()
    var previous

    previous = metadata.setSampleFlagTo(0)
    expect(metadata.toString().slice(0, -2)).equal(md.slice(0, -2))
    expect(metadata.toString().slice(-2)).equal('00')
    expect(previous).equal(onEntry)

    previous = metadata.setSampleFlagTo(1)
    expect(metadata.toString().slice(0, -2)).equal(md.slice(0, -2))
    expect(metadata.toString().slice(-2)).equal('01')
    expect(previous).equal(false)

    previous = metadata.setSampleFlagTo(onEntry)
    expect(previous).equal(true)
  })

  it('should construct using metadata', function () {
    var md = new bindings.Metadata(metadata)
    expect(md.toString()).equal(metadata.toString())
  })

  it('should construct using an event', function () {
    var e = new bindings.Event(metadata)
    var md = new bindings.Metadata(e)
    expect(md.toString()).equal(e.toString())
  })

  it('should serialize to string', function () {
    string = metadata.toString()
    expect(string).not.equal('')
  })

  it('should construct from string', function () {
    metadata = bindings.Metadata.fromString(string)
    expect(metadata.toString()).equal(string)
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
      expect(correct).equal(true, k)
    })

  })

  it('should correctly test for the sample flag', function () {
    var mdNoSample = bindings.Metadata.makeRandom()
    var mdSample = bindings.Metadata.makeRandom(1)
    expect(bindings.Metadata.sampleFlagIsSet(mdNoSample)).equal(false)
    expect(bindings.Metadata.sampleFlagIsSet(mdSample)).equal(true)

    var e = new bindings.Event(mdNoSample)
    expect(bindings.Metadata.sampleFlagIsSet(e)).equal(false)
    expect(bindings.Metadata.sampleFlagIsSet(e.toString())).equal(false)

    e = new bindings.Event(mdSample)
    expect(bindings.Metadata.sampleFlagIsSet(e)).equal(true)
    expect(bindings.Metadata.sampleFlagIsSet(e.toString())).equal(true)

    // if there is an error it should return undefined.
    var result = typeof bindings.Metadata.sampleFlagIsSet('')
    expect(result).equal('undefined')
  })

  it.skip('should clone itself', function () {
    var rand = bindings.Metadata.makeRandom()
    rand.copy().toString().should.equal(rand.toString())
  })

  it('should be valid', function () {
    expect(metadata.isValid()).equal(true)
  })

  it.skip('should create an event', function () {
    var event = metadata.createEvent()
    expect(event).instanceof(bindings.Event)
  })

  it('should not crash node when getting the prototype of an metadata instance', function () {
    var p = Object.getPrototypeOf(bindings.Metadata.makeRandom())
  })
})
