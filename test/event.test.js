var bindings = require('../')

describe('addon.event', function () {
  var event
  var random = bindings.Metadata.makeRandom()
  var randomSample = bindings.Metadata.makeRandom(1)

  bindings.oboeInit(process.env.APPOPTICS_SERVICE_KEY)

  it('should construct an event', function () {
    event = new bindings.Event()
  })

  it('should construct an event using metadata', function () {
    var beginning = random.toString().slice(0, 42)
    event = new bindings.Event(random)
    // They should share the same task ID
    event.toString().slice(0, 42).should.equal(beginning)
    // they should have different op IDs.
    event.toString().slice(42, 58).should.not.equal(random.toString().slice(42, 58))
    event.getSampleFlag().should.equal(false)
  })

  it('should construct an event with the sample flag set', function () {
    var event = new bindings.Event(randomSample)
    event.getSampleFlag().should.equal(true)
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
    // it seems that .should.throw() doesn't work quite right.
    var threw = false
    try {
      e.addEdge(md)
    } catch (e) {
      threw = true
    }
    threw.should.equal(true)
  })

  it('should get metadata', function () {
    var meta = event.getMetadata()
    meta.should.be.an.instanceof(bindings.Metadata)
  })

  it('should serialize metadata to id string', function () {
    var meta = event.toString()
    meta.should.be.an.instanceof(String).with.lengthOf(60)
    meta.should.match(/2B[A-F0-9]{58}/)
  })

  it('should set, clear, and return previous sample flag', function () {
    var e = event.toString()
    var onEntry = event.getSampleFlag()
    var previous

    previous = event.setSampleFlagTo(0)
    event.toString().slice(0, -2).should.equal(e.slice(0, -2))
    event.toString().slice(-2).should.equal('00')
    previous.should.equal(onEntry)

    previous = event.setSampleFlagTo(1)
    event.toString().slice(0, -2).should.equal(e.slice(0, -2))
    event.toString().slice(-2).should.equal('01')
    previous.should.equal(false)

    previous = event.setSampleFlagTo(onEntry)
    previous.should.equal(true)
  })

  it('should create an event without adding an edge', function () {
    var event2 = new bindings.Event(bindings.Metadata.makeRandom(), false)
  })

  it('should not crash node when getting the prototype of an event', function () {
    var p = Object.getPrototypeOf(event)
  })
})
