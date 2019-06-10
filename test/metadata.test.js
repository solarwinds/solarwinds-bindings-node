var bindings = require('../')
const expect = require('chai').expect

describe('addon.metadata', function () {
  var metadata
  var string
  const serviceKey = `${process.env.AO_TOKEN_PROD}:node-bindings-test`;

  it('should initialize oboe with only a service key', function () {
    const status = bindings.oboeInit({serviceKey})
    // kind of funky but -1 is already initialized, 0 is ok. mocha runs
    // multiple tests in one process so the result is 0 if run standalone
    // but -1 on all but the first if run as a suite.
    expect(status).oneOf([-1, 0]);
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

  it('should be valid', function () {
    expect(metadata.isValid()).equal(true)
  })

  it('should format metadata correctly for all options', function () {
    // 2B 123456789ABCDEF0123456789ABCDEF012345678 FEDCBA9876543210 01
    const h = '2B';
    const t = '123456789ABCDEF0123456789ABCDEF012345678';
    const o = 'FEDCBA9876543210';
    const f = '01';

    //const mds = '2B123456789ABCDEF0123456789ABCDEF012345678FEDCBA987654321001';
    const mds = `${h}${t}${o}${f}`;
    const md = bindings.Metadata.fromString(mds);
    // simple formatting
    let s = md.toString();
    expect(s).equal(mds);
    // the human readable version i use for testing and logging. this is special-cased in the code
    // because the 1 bit really means display the header (2B) but because that's not useful and 1
    // was historically used to display colon-separated lowercase trace ids, i'm keeping it that way.
    s = md.toString(1);
    expect(s).equal(`${h}-${t}-${o}-${f}`.toLowerCase());

    // if there is an argument and it is not 1 then the argument is bit flags specifying the format.
    // 1 header
    // 2 task id
    // 4 op id
    // 8 flags - display whole flags byte (if this is present then the 16 sample bit is ignored)
    // 16 sample bit - display only sample bit
    // 32 separators - insert dash separators between elements
    // 64 lowercase alpha characters

    // reproduce simple formatting with explicit bits
    s = md.toString(1 + 2 + 4 + 8);
    expect(s).equal(mds);
    // the human readable form, explicitly specified but removing lowercase
    s = md.toString(bindings.Metadata.fmtHuman ^ 64);
    expect(s).equal(`${h}-${t}-${o}-${f}`);
    // the format to link to traces in logs
    s = md.toString(bindings.Metadata.fmtLog);
    expect(s).equal(`${t}-1`);
    // and let's make that easier to look at so we can add the option later
    s = md.toString(bindings.Metadata.fmtLog | 64);
    expect(s).equal(`${t}-1`.toLowerCase());
  })

  it('should not crash node when getting the prototype of an metadata instance', function () {
    var p = Object.getPrototypeOf(bindings.Metadata.makeRandom())
  })
})
