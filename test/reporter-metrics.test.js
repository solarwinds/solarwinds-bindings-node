/* global describe, before, it */
'use strict'

const bindings = require('../')
const expect = require('chai').expect

const maxIsReadyToSampleWait = 60000

describe('bindings.Reporter.sendMetrics()', function () {
  before(function () {
    const serviceKey = `${process.env.AO_TOKEN_NH}:node-bindings-test`
    const endpoint = `${process.env.APPOPTICS_COLLECTOR_NH}`

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

  it('should require an array argument', function () {
    const args = [
      { i: 'am not an array' },
      'nor am i',
      0,
      false,
      Symbol('xyzzy')
    ]

    for (let i = 0; i < args.length; i++) {
      function test () {
        bindings.Reporter.sendMetrics(args[i])
      }
      expect(test).throws('invalid signature for sendMetrics()')
    }
  })

  it('should handle an array of zero length', function () {
    const status = bindings.Reporter.sendMetrics([])
    expect(status).deep.equal({ errors: [] })
  })

  it('should recognize and return bad metrics', function () {
    const x = Symbol('expected-error')
    const metrics = [
      'not an object',
      [],
      { count: 1, [x]: 'must have string name' },
      { name: 'gc.count', count: 'x', [x]: 'count must be a number' },
      { name: 'gc.count', count: -1, [x]: 'count must be greater than 0' },
      { name: 'gc.count', count: 1, value: 'x', [x]: 'summary value must be numeric' },
      { name: 'gc.count', count: 1, value: 3.141592654, tags: 'i am not an object', [x]: 'tags must be plain object' },
      { name: 'gc.count', count: 1, value: 11235813, tags: ['i am not a plain object'], [x]: 'tags must be plain object' }
    ]

    const results = bindings.Reporter.sendMetrics(metrics)
    expect(results).not.property('correct')
    expect(results).property('errors').an('array')
    expect(results.errors.length).equal(metrics.length, 'all metrics should be errors')

    for (let i = 0; i < metrics.length; i++) {
      const error = results.errors[i]
      let expectedError
      if (typeof metrics[i] !== 'object' || Array.isArray(metrics[i])) {
        expectedError = 'metric must be plain object'
      } else {
        expectedError = error.metric[x]
        delete error.metric[x]
      }

      expect(error.code).equal(expectedError)
      expect(error.metric).deep.equal(metrics[i])
    }
  })

  it('should handle correct metrics', function () {
    const metrics = [
      { name: 'testing.node.123' },
      { name: 'testing.node.abc', count: 1 },
      { name: 'testing.node.def', count: 1, value: 42 },
      { name: 'testing.node.xyz', count: 1, value: 84, addHostTag: true },
      { name: 'testing.node.xyz', count: 1, value: 2.71828, addHostTag: false },
      { name: 'testing.node.gc', count: 4, tags: { bruce: 'test', macnaughton: 'name' } },
      { name: 'testing.node.bool', count: 1, addHostTag: 1 },
      { name: 'testing.node.bool', count: 1, addHostTag: '' }
    ]
    // make this a noop so that if run following the memory tests that it won't fail
    // due to the memory tests having overloaded oboe.
    const results = bindings.Reporter.sendMetrics(metrics, { testing: true, noop: true })
    expect(results).property('correct').an('array')
    expect(results.correct.length).equal(metrics.length)
    expect(results).property('errors').an('array')
    expect(results.errors.length).equal(0)

    for (let i = 0; i < results.correct.length; i++) {
      const metric = results.correct[i]
      const defaults = { count: 1 }
      const addedOptions = { addHostTag: !!metrics[i].addHostTag }
      const expected = Object.assign(defaults, metrics[i], addedOptions)
      expect(metric).deep.equal(expected)
    }
  })
})
