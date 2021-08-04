/* global describe, before, it */
'use strict'

const bindings = require('../')
const expect = require('chai').expect

const env = process.env
const maxIsReadyToSampleWait = 60000

describe('addon.settings', function () {
  before(function () {
    const serviceKey = process.env.APPOPTICS_SERVICE_KEY || `${env.AO_TOKEN_STG}:node-bindings-test`
    const endpoint = process.env.APPOPTICS_COLLECTOR || 'collector-stg.appoptics.com'

    this.timeout(maxIsReadyToSampleWait)
    const status = bindings.oboeInit({ serviceKey, endpoint })
    // oboeInit can return -1 for already initialized or 0 if succeeded.
    // depending on whether this is run as part of a suite or standalone
    // either result is valid.
    if (status !== -1 && status !== 0) {
      throw new Error('oboeInit() failed')
    }

    const ready = bindings.isReadyToSample(maxIsReadyToSampleWait)
    expect(ready).equal(1, `should be connected to ${endpoint} and ready`)
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
    let threw = false
    try {
      bindings.Settings.setDefaultSampleRate(-1)
      bindings.Settings.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE + 1)
    } catch (e) {
      threw = true
    }

    expect(threw).equal(false, 'setting invalid sample rate must throw')
  })

  it('should handle bad sample rates correctly', function () {
    let rateUsed
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
    const ev0 = bindings.Event.makeRandom(0)
    const event = new bindings.Event(ev0)
    const xtrace = event.toString()
    const settings = bindings.Settings.getTraceSettings({ xtrace })

    expect(settings).property('message', 'ok')
    expect(settings).property('status', -1) // -1 means non-sampled xtrace
    expect(settings).property('doSample', false)
    expect(settings.metadata.toString()).equal(xtrace)
  })

  it('should get verification that a request should be sampled', function (done) {
    bindings.Settings.setTracingMode(bindings.TRACE_ALWAYS)
    bindings.Settings.setDefaultSampleRate(bindings.MAX_SAMPLE_RATE)
    const event = new bindings.Event(bindings.Event.makeRandom(1))
    const xtraceString = event.toString()
    let counter = 20
    // test for old behavior and "is-continued" behavior
    const [maj, min] = bindings.Config.getVersionString().split('.').map(n => Number(n))
    const negative = (maj == 10 && min >= 1) || (maj > 10) // eslint-disable-line eqeqeq
    // poll to give time for the SSL connection to complete. it should have
    // been waited on in before() but it's possible for the connection to break.
    const id = setInterval(function () {
      const settings = bindings.Settings.getTraceSettings({ xtrace: xtraceString })
      if (--counter <= 0 || typeof settings === 'object' && settings.source !== (negative ? -1 : 2)) { // eslint-disable-line no-mixed-operators
        clearInterval(id)
        expect(settings).property('status', 0)
        expect(settings).property('doSample', true)
        expect(settings).property('doMetrics', true)
        expect(settings).property('edge', true)
        expect(settings.metadata).instanceof(bindings.Event)
        expect(settings).property('rate', negative ? -1 : bindings.MAX_SAMPLE_RATE)
        expect(settings).property('tokenBucketRate').exist // eslint-disable-line no-unused-expressions
        expect(settings).property('tokenBucketCapacity').exist // eslint-disable-line no-unused-expressions
        // the following depends on whether this suite is run standalone or with other
        // test files.
        if (negative) {
          expect(settings.source).equal(-1)
        } else {
          expect(settings.source).oneOf([1, 6])
        }

        if (counter < 0) {
          done(new Error('getTraceSettings() never returned valid settings'))
        } else {
          done()
        }
      }
    }, 50)
  })

  it('should not set sample bit unless specified', function () {
    const md0 = bindings.Event.makeRandom(0)
    const md1 = bindings.Event.makeRandom(1)

    let event = new bindings.Event(md0)
    expect(event.getSampleFlag()).equal(false)
    expect(event.toString().slice(-2)).equal('00')

    event = new bindings.Event(md1)
    expect(event.getSampleFlag()).equal(true)
    expect(event.toString().slice(-2)).equal('01')
  })
})
