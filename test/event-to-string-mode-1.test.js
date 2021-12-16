/* global describe, before, it */
'use strict'

/* eslint-disable new-cap  */

const bindings = require('../')
const expect = require('chai').expect

const env = process.env
const maxIsReadyToSampleWait = 60000

describe('bindings.Event.toString() mode 1', function () {
  before(function () {
    const serviceKey = process.env.SOLARWINDS_SERVICE_KEY || `${env.AO_TOKEN_NH}:node-bindings-test`
    const endpoint = process.env.SOLARWINDS_COLLECTOR || `${env.APPOPTICS_COLLECTOR_NH}`

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

  /*
    from bindings.h

    const static int ff_header = 1;
    const static int ff_task = 2;
    const static int ff_op = 4;
    const static int ff_flags = 8;
    const static int ff_sample = 16;
    const static int ff_separators = 32;
    const static int ff_lowercase = 64; // in mode 1 everything is always lowervase

  */
  it('should return traceparent when no argument is provided', function () {
    const xtraceData = new bindings.Event.makeRandom()
    const event = new bindings.Event(xtraceData)

    expect(event.toString().length).equal(55)
    expect(event.toString().slice(0, 2)).equal('00')
    expect(event.toString().slice(-2)).equal('00')

    expect(event.toString().split('-').length).equal(4)
    expect(event.toString().split('-')[1].length).equal(32)
    expect(event.toString().split('-')[2].length).equal(16)
  })

  // see event-to-string comment
  it('should return a lowercase delimited xtrace when argument is 1', function () {
    const xtraceData = new bindings.Event.makeRandom()
    const event = new bindings.Event(xtraceData)
    const data = event.toString()

    const taskId = data.split('-')[1]
    const opId = data.split('-')[2]
    const flags = data.split('-')[3]

    expect(event.toString(1)).equal(`00-${taskId}-${opId}-${flags}`)
  })

  it('should return just task id when argument is 2', function () {
    const xtraceData = new bindings.Event.makeRandom()
    const event = new bindings.Event(xtraceData)
    const data = event.toString()

    const taskId = data.split('-')[1]

    expect(event.toString(2)).equal(taskId)
  })

  it('should return just op id when argument is 4', function () {
    const xtraceData = new bindings.Event.makeRandom()
    const event = new bindings.Event(xtraceData)
    const data = event.toString()

    const opId = data.split('-')[2]

    expect(event.toString(4)).equal(opId)
  })

  it('should return just flags when argument is 8', function () {
    const xtraceData = new bindings.Event.makeRandom()
    const event = new bindings.Event(xtraceData)
    const data = event.toString()

    const flags = data.split('-')[3]

    expect(event.toString(8)).equal(flags)
  })

  it('should return just the sample byte when argument is 16', function () {
    const xtraceData = new bindings.Event.makeRandom()
    const event = new bindings.Event(xtraceData)
    const data = event.toString()

    const byte = data.slice(-1)

    expect(event.toString(16)).equal(byte)
  })

  it('should return a lowercased parts when argument is 66', function () {
    const xtraceData = new bindings.Event.makeRandom()
    const event = new bindings.Event(xtraceData)
    const data = event.toString()

    const taskId = data.split('-')[1]

    expect(event.toString(66)).equal(taskId.toLowerCase())
  })

  it('should return a lowercased parts when argument is 68', function () {
    const xtraceData = new bindings.Event.makeRandom()
    const event = new bindings.Event(xtraceData)
    const data = event.toString()

    const opId = data.split('-')[2]

    expect(event.toString(68)).equal(opId.toLowerCase())
  })

  it('should return a delimited opIdflags when argument is 108', function () {
    const xtraceData = new bindings.Event.makeRandom()
    const event = new bindings.Event(xtraceData)
    const data = event.toString()

    const opId = data.split('-')[2]
    const flags = data.split('-')[3]

    expect(event.toString(108)).equal(`${opId.toLowerCase()}-${flags}`)
  })
})
