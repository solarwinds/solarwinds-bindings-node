/* global describe, before, it */
'use strict'

const bindings = require('../')
const expect = require('chai').expect

const maxIsReadyToSampleWait = 60000

const evUnsampled = '00-5bd5777ca0077c734b537b64c6b96921-1aa0b1b42979f5c4-00'
const evSampled = '00-4fc9017ba3404828f253638a697dc7cf-a544d5b98159b555-01'

describe('bindings.Event mode 1', function () {
  before(function () {
    const serviceKey = process.env.SW_APM_SERVICE_KEY
    const endpoint = process.env.SW_APM_COLLECTOR

    this.timeout(maxIsReadyToSampleWait)
    const status = bindings.oboeInit({ serviceKey, endpoint, mode: 1 })
    // oboeInit can return -1 for already initialized or 0 if succeeded.
    // depending on whether this is run as part of a suite or standalone
    // either result is valid.
    if (status !== -1 && status !== 0) {
      throw new Error('oboeInit() failed')
    }

    const ready = bindings.isReadyToSample(maxIsReadyToSampleWait)
    expect(ready).equal(1, `should be connected to ${endpoint} and ready`)
  })

  it('should throw if the constructor is called without "new"', function () {
    function badCall () {
      bindings.Event()
    }
    expect(badCall, 'should throw without "new"').throws(TypeError, 'Class constructors cannot be invoked without \'new\'')
  })

  it('should succeed when called with no arguments', function () {
    const event = new bindings.Event()
    expect(event).instanceof(bindings.Event)
  })

  it('should throw if the constructor is called without an event', function () {
    const tests = [
      { args: [1], text: 'non-metadata', e: 'invalid signature' },
      { args: ['invalid-x-trace-string'], text: 'invalid x-trace', e: 'invalid signature' }
    ]

    for (const t of tests) {
      function badCall () {
        new bindings.Event(...t.args) // eslint-disable-line no-new
      }
      expect(badCall, `${t.text} should throw "${t.e}"`).throws(TypeError, t.e)
    }
  })

  it('should construct an event using an event', function () {
    const random = bindings.Event.makeRandom()
    const partsRandom = random.toString().split('-')

    const event = new bindings.Event(random)
    const partsEvent = event.toString().split('-')

    expect(partsRandom[0]).equal(partsEvent[0].toString())
    expect(partsRandom[1]).equal(partsEvent[1].toString())
    expect(partsRandom[3]).equal(partsEvent[3].toString())
  })

  it('should construct an event with or without an edge', function () {
    // note: not actually looking inside the event
    const random = bindings.Event.makeRandom()
    const partsRandom = random.toString().split('-')

    const tests = [
      { args: [random, false], text: 'no edge' },
      { args: [random, true], text: 'edge' }
    ]

    for (const t of tests) {
      const event = new bindings.Event(...t.args)
      const partsEvent = event.toString().split('-')
      // just compare the header, task, and flags. op id is different.
      expect(partsRandom[0]).equal(partsEvent[0].toString())
      expect(partsRandom[1]).equal(partsEvent[1].toString())
      expect(partsRandom[3]).equal(partsEvent[3].toString())
    }
  })

  it('should construct an event using a random template', function () {
    const random = bindings.Event.makeRandom()
    const partsRandom = random.toString().split('-')

    const event = new bindings.Event(random)
    const partsEvent = event.toString().split('-')
    // They should have the same task header
    expect(partsRandom[1]).equal(partsEvent[1])
    // They should have the same task ID
    expect(partsRandom[1]).equal(partsEvent[1])
    // they should have different op IDs.
    expect(partsRandom[2]).not.equal(partsEvent[2])
    expect(event.getSampleFlag()).equal(false)
  })

  it('should construct an event with the sample flag set', function () {
    const ev1 = bindings.Event.makeRandom(1)
    const ev2 = new bindings.Event(ev1)
    expect(ev1.toString().slice(-2)).equal('01')
    expect(ev2.toString().slice(-2)).equal('01')
    expect(ev1.getSampleFlag()).equal(true)
    expect(ev2.getSampleFlag()).equal(true)
  })

  it('shouldn\'t throw when adding a valid KV using addInfo', function () {
    const event = new bindings.Event(bindings.Event.makeRandom())
    event.addInfo('key', 'val')
  })

  it('shouldn\'t throw when adding an edge from an event', function () {
    const event = new bindings.Event(bindings.Event.makeRandom())
    const edge = new bindings.Event(event)
    event.addEdge(edge)
  })

  it('should add edge from a string', function () {
    const event = new bindings.Event(bindings.Event.makeRandom())
    const edge = new bindings.Event(event).toString()
    event.addEdge(edge)
  })

  it('should not add edge when task IDs do not match', function () {
    const event = new bindings.Event(bindings.Event.makeRandom())
    const edge = bindings.Event.makeRandom()
    expect(edge.toString(2)).not.equal(event.toString(2))

    // force event to a different task ID because oboe returns "OK"
    // when the event metadata is invalid. And just calling
    // new Event() results in 2B0000000..0000<op id><flags>, i.e.,
    // invalid metadata. So it doesn't fail even though it didn't
    // add an edge. Hmmm.
    let threw = false
    try {
      event.addEdge(edge)
    } catch (e) {
      threw = true
    }
    expect(threw).equal(true)
  })

  it('Event.makeFromString() should work', function () {
    const ev0 = bindings.Event.makeFromString(evUnsampled)
    const ev1 = bindings.Event.makeFromString(evSampled)

    expect(ev0.getSampleFlag()).equal(false)
    expect(ev0.toString()).equal(evUnsampled)
    expect(ev1.getSampleFlag()).equal(true)
    expect(ev1.toString()).equal(evSampled)
  })

  it('should serialize an event string', function () {
    const ev1 = bindings.Event.makeFromString(evUnsampled)
    const ev2 = new bindings.Event(ev1)
    // task IDs should match
    expect(ev2.toString(2)).equal(ev1.toString(2))
    // op IDs should not match
    expect(ev2.toString(4)).not.equal(ev1.toString(4))
    expect(ev2.toString().substr(0, 2)).equal('00')
    expect(ev2.toString().slice(-2)).equal('00')
    expect(ev2.toString().length).equal(55)
    expect(ev2.toString()).match(/\b00-[0-9a-f]{32}-[0-9a-f]{16}-[0-9a-f]{2}\b/)
  })

  it('should create an event with or without adding an edge', function () {
    const ev1 = new bindings.Event(bindings.Event.makeRandom(), false)
    const ev2 = new bindings.Event(bindings.Event.makeRandom(), true)
    expect(ev1).instanceof(bindings.Event)
    expect(ev2).instanceof(bindings.Event)
  })

  it('should not crash node when getting the prototype of an event', function () {
    const event = bindings.Event.makeRandom()
    // eslint-disable-next-line no-unused-vars
    const p = Object.getPrototypeOf(event)
  })

  it('should fetch event stats', function () {
    const stats = bindings.Event.getEventStats()
    const expectedStats = [
      'totalCreated',
      'totalDestroyed',
      'totalActive',
      'smallActive',
      'fullActive',
      'bytesUsed',
      'sentCount',
      'lifetime',
      'averageLifetime',
      'bytesFreed',
      'totalBytesAllocated',
      'sendtime',
      'averageSendtime'
    ]
    expect(Object.keys(stats)).members(expectedStats)
  })

  it('makeRandom() should allocate a small event', function () {
    const event = new bindings.Event.makeRandom() // eslint-disable-line new-cap
    const bytes = event.getBytesAllocated()
    expect(bytes).equal(224, 'sizeof(oboe_event_t) should be 224')
  })

  it('makeFromString() should allocate a small event', function () {
    let event = new bindings.Event.makeFromString(evUnsampled) // eslint-disable-line new-cap
    let bytes = event.getBytesAllocated()
    expect(bytes).equal(224, 'sizeof(oboe_event_t) should be 224')

    event = new bindings.Event.makeFromString(evSampled) // eslint-disable-line new-cap
    bytes = event.getBytesAllocated()
    expect(bytes).equal(224, 'sizeof(oboe_event_t) should be 224')
  })

  it('new Event() should allocate a full event', function () {
    const xtraceData = new bindings.Event.makeRandom() // eslint-disable-line new-cap
    const event = new bindings.Event(xtraceData)
    const bytes = event.getBytesAllocated()
    expect(bytes).equal(224 + 1024, 'should include a 1024 byte buffer')
  })
})
