/* global describe, it */
'use strict'

const bindings = require('../')
const expect = require('chai').expect

const env = process.env
const maxIsReadyToSampleWait = 60000

const EnvVarOptions = require('./lib/env-var-options')
const keyMap = require('./lib/env-var-key-map')

//
// goodOptions are used in multiple tests so they're declared here
//
const goodOptions = {
  serviceKey: 'magical-service-key',
  trustedPath: 'secret-garden',
  hostnameAlias: 'incognito',
  logLevel: 3,
  reporter: 'udp',
  endpoint: 'localhost:9999',
  tokenBucketCapacity: 100,
  tokenBucketRate: 100,
  bufferSize: 100,
  // logFilePath: 'where-to-write',  // not supported by the node agent
  traceMetrics: true,
  histogramPrecision: 100,
  maxTransactions: 100,
  maxFlushWaitTime: 100,
  eventsFlushInterval: 100,
  eventsFlushBatchSize: 100,
  // oneFilePerEvent: 1,             // not supported by the node agent
  ec2MetadataTimeout: 3000,
  proxy: 'http://proxy-host:10101',
  mode: 0
}

describe('bindings.oboeInit()', function () {
  // Important: when only services key is provided the host is set to production by default
  // tests here always run against production collector. no way to "override" with settings.

  it('should initialize oboe with only a service key', function () {
    const result = bindings.oboeInit({ serviceKey: `${env.SW_TEST_PROD_SERVICE_KEY}`, mode: 1 })
    // oboeInit can return -1 for already initialized or 0 if succeeded.
    // depending on whether this is run as part of a suite or standalone
    // either result is valid.
    expect(result).oneOf([-1, 0])
  })

  it('should check if ready to sample using collector', function () {
    this.timeout(maxIsReadyToSampleWait)
    const ready = bindings.isReadyToSample(maxIsReadyToSampleWait)
    expect(ready).be.a('number')

    expect(ready).equal(1, 'collector should be ready')
  })

  it('should handle good options values', function () {
    const details = {}
    const options = Object.assign({}, goodOptions)
    const expected = options
    const result = bindings.oboeInit(options, details)

    expect(result).lte(0, 'the first oboeInit() should succeed')
    expect(details.valid).deep.equal(expected)
    expect(details.processed).deep.equal(expected)

    const keysIn = Object.keys(options)
    const keysOut = Object.keys(details.processed)
    expect(keysIn.length).equal(keysOut.length)
  })

  it('should handle bad options values', function () {
    const badOptions = {
      serviceKey: null,
      trustedPath: null,
      hostnameAlias: null,
      logLevel: 'a',
      reporter: null,
      endpoint: null,
      tokenBucketCapacity: 'a',
      tokenBucketRate: 'a',
      bufferSize: 'a',
      // logFilePath: 0,
      traceMetrics: 0,
      histogramPrecision: 'a',
      maxTransactions: 'a',
      maxFlushWaitTime: 'a',
      eventsFlushInterval: 'a',
      eventsFlushBatchSize: 'a',
      // oneFilePerEvent: undefined,
      ec2MetadataTimeout: 'x',
      proxy: 17
    }

    // any value can be interpreted as a boolean so they should come
    // back as valid. but all others should be invalid.
    const booleans = {
      traceMetrics: 0
      // oneFilePerEvent: undefined
    }
    const details = {}
    const options = Object.assign({}, badOptions)

    // it's already init'd so expect a -1 result.
    const result = bindings.oboeInit(options, details)
    expect(result).equal(-1, 'oboe should already be initialized')
    expect(details.valid).deep.equal(booleans)
  })

  it('should handle the environment variable values', function () {
    const details = {}
    const envVars = new EnvVarOptions('SW_APM_')
    const options = envVars.getConverted(keyMap)

    const result = bindings.oboeInit(options, details)
    expect(result).equal(-1, 'oboe should already be initialized')
    expect(details.processed).deep.equal(options, 'env vars should be processed')
  })

  it('should not include invalid properties in details.valid', function () {
    const invalidOptions = {
      serviceKey: 'magical-key',
      serviceLock: 'gordian lock',
      xyzzy: 'plover'
    }
    const details = {}
    const options = Object.assign({}, invalidOptions)

    const result = bindings.oboeInit(options, details)
    expect(result).equal(-1, 'oboe should already be initialized')
    expect(details.valid).deep.equal({ serviceKey: invalidOptions.serviceKey })
  })

  it('should throw if not passed an object', function () {
    try { bindings.oboeInit() } catch (err) {
      expect(err.message).eq('invalid calling signature')
    }
  })

  it('should throw if passed empty object', function () {
    try { bindings.oboeInit({}) } catch (err) {
      expect(err.message).eq('invalid calling signature')
    }
  })
})
