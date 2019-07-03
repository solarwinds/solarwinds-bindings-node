var bindings = require('../')
const expect = require('chai').expect

describe('addon.event', function () {
  var event
  var random
  var randomSample
  const serviceKey = `${process.env.AO_TOKEN_PROD}:node-bindings-test`;

  it('should initialize oboe with only a service key', function () {
    const status = bindings.oboeInit({serviceKey})
    // kind of funky but -1 is already initialized, 0 is ok. mocha runs
    // multiple tests in one process so the result is 0 if run standalone
    // but -1 on all but the first if run as a suite.
    expect(status).oneOf([-1, 0]);
    random = bindings.Metadata.makeRandom()
    randomSample = bindings.Metadata.makeRandom(1)
  })

  it('should throw if the constructor is called without "new"', function () {
    function badCall () {
      bindings.Event();
    }
    expect(badCall, 'should throw without "new"').throws(TypeError, 'Class constructors cannot be invoked without \'new\'');
  })

  it('should throw if the constructor is called with bad arguments', function () {
    const random = bindings.Metadata.makeRandom();

    const tests = [
      {args: [], text: 'no args', e: 'invalid signature'},
      {args: [1], text: 'non-metadata', e: 'no metadata found'},
      {args: ['invalid-x-trace-string'], text: 'invalid x-trace', e: 'Event::New - invalid X-Trace ID string'},
    ];

    for (let t of tests) {
      function badCall () {
        new bindings.Event(...t.args);
      }
      expect(badCall, `${t.text} should throw "${t.e}"`).throws(TypeError, t.e);
    }
  })

  it('should construct an event using metadata', function () {
    const fmt = 1|2|8;        // header, task, flags
    const metadata = bindings.Metadata.makeRandom();
    const event = new bindings.Event(metadata);
    expect(event.toString(fmt)).equal(metadata.toString(fmt));
  })

  it('should construct an event using any form of metadata', function () {
    const fmt = 1|2|8;        // just compare the header, task, and flags.
    const metadata = bindings.Metadata.makeRandom();
    const event = new bindings.Event(metadata);

    const tests = [
      {args: [metadata], text: 'metadata'},
      {args: [event], text: 'an event'},
      {args: [metadata.toString()], text: 'a string'},
      {args: [metadata, false], text: 'metadata, no edge'},
      {args: [metadata, true], text: 'metadata, edge'},
    ];

    for (let t of tests) {
      expect(event.toString(fmt), `from "${t.text}"`).equal(metadata.toString(fmt));
    }
  })

  it('should construct an event using metadata', function () {
    var beginning = random.toString().slice(0, 42)
    event = new bindings.Event(random)
    // They should share the same task ID'
    expect(event.toString().slice(0, 42)).equal(beginning)
    // they should have different op IDs.
    expect(event.toString().slice(42, 58)).not.equal(random.toString().slice(42, 58))
    expect(event.getSampleFlag()).equal(false)
  })

  it('should construct an event with the sample flag set', function () {
    var event = new bindings.Event(randomSample)
    expect(event.getSampleFlag()).equal(true)
  })

  it('should add info', function () {
    event.addInfo('key', 'val')
  })

  it('should add edge from metadata', function () {
    // base the metadata on the same task ID
    var meta = new bindings.Metadata(random);
    event.addEdge(meta)
  })

  it('should add edge as from an event', function () {
    var e = new bindings.Event(event)
    event.addEdge(e)
  })

  it('should add edge from a string', function () {
    var e = new bindings.Event(event)
    var meta = e.toString()
    event.addEdge(meta.toString())
  })

  it('should not add edge when task IDs do not match', function () {
    var md = bindings.Metadata.makeRandom()
    // force event to a different task ID because oboe returns "OK"
    // when the event metadata is invalid. And just calling
    // new Event() results in 2B0000000..0000<op id><flags>, i.e.,
    // invalid metadata. So it doesn't fail even though it didn't
    // add an edge. Hmmm.
    var e = new bindings.Event(bindings.Metadata.makeRandom())
    var threw = false
    try {
      e.addEdge(md)
    } catch (e) {
      threw = true
    }
    expect(threw).equal(true)
  })

  it('should get metadata', function () {
    var meta = event.getMetadata()
    expect(meta).instanceOf(bindings.Metadata)
  })

  it('should serialize metadata to id string', function () {
    var meta = event.toString()
    expect(meta).be.a('string').with.lengthOf(60)
    expect(meta).match(/2B[A-F0-9]{58}/)
  })

  it('should set, clear, and return previous sample flag', function () {
    var e = event.toString()
    var onEntry = event.getSampleFlag()
    var previous

    previous = event.setSampleFlagTo(0)
    expect(event.toString().slice(0, -2)).equal(e.slice(0, -2))
    expect(event.toString().slice(-2)).equal('00')
    expect(previous).equal(onEntry)

    previous = event.setSampleFlagTo(1)
    expect(event.toString().slice(0, -2)).equal(e.slice(0, -2))
    expect(event.toString().slice(-2)).equal('01')
    expect(previous).equal(false)

    previous = event.setSampleFlagTo(onEntry)
    expect(previous).equal(true)
  })

  it('should create an event without adding an edge', function () {
    var event2 = new bindings.Event(bindings.Metadata.makeRandom(), false)
  })

  it('should not crash node when getting the prototype of an event', function () {
    var p = Object.getPrototypeOf(event)
  })
})
