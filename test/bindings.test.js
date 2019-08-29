const bindings = require('../')
const expect = require('chai').expect;
const util = require('util');

const EnvVarOptions = require('./lib/env-var-options');
const keyMap = require('./lib/env-var-key-map');

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
  //logFilePath: 'where-to-write',  // not supported by the node agent
  traceMetrics: true,
  histogramPrecision: 100,
  maxTransactions: 100,
  maxFlushWaitTime: 100,
  eventsFlushInterval: 100,
  eventsFlushBatchSize: 100,
  //oneFilePerEvent: 1,             // not supported by the node agent
  ec2MetadataTimeout: 3000,
}

describe('addon.oboeInit()', function () {

  it('should handle good options values', function () {
    const booleanOptions = {
      traceMetrics: true,
      //oneFilePerEvent: true
    }

    const details = {};
    const options = Object.assign({}, goodOptions);
    const expected = options;
    const result = bindings.oboeInit(options, details);

    expect(result).lte(0, 'the first oboeInit() should succeed');
    expect(details.valid).deep.equal(expected);
    expect(details.processed).deep.equal(expected);

    const keysIn = Object.keys(options);
    const keysOut = Object.keys(details.processed);
    expect(keysIn.length).equal(keysOut.length);
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
      //logFilePath: 0,
      traceMetrics: 0,
      histogramPrecision: 'a',
      maxTransactions: 'a',
      maxFlushWaitTime: 'a',
      eventsFlushInterval: 'a',
      eventsFlushBatchSize: 'a',
      //oneFilePerEvent: undefined,
      ec2MetadataTimeout: 'x',
    }

    // any value can be interpreted as a boolean so they should come
    // back as valid.
    const booleans = {
      traceMetrics: 0,
      //oneFilePerEvent: undefined
    }
    const details = {};
    const options = Object.assign({}, badOptions);

    // it's already init'd so expect a -1 result.
    var result = bindings.oboeInit(options, details);
    expect(result).equal(-1, 'oboe should already be initialized');
    expect(details.valid).deep.equal(booleans);
  })

  it('should handle the environment variable values', function () {
    const details = {};
    const envVars = new EnvVarOptions('APPOPTICS_');
    const options = envVars.getConverted(keyMap);

    const result = bindings.oboeInit(options, details);
    expect(result).equal(-1, 'oboe should already be initialized');
    expect(details.processed).deep.equal(options, 'env vars should be processed');
  })

  it('should not include invalid properties in details.valid', function () {
    const invalidOptions = {
      serviceKey: 'magical-key',
      serviceLock: 'gordian lock',
      xyzzy: 'plover',
    }
    const details = {};
    const options = Object.assign({}, invalidOptions);

    const result = bindings.oboeInit(options, details);
    expect(result).equal(-1, 'oboe should already be initialized');
    expect(details.valid).deep.equal({serviceKey: invalidOptions.serviceKey});
  })

  it('should throw if not passed an object', function () {
    expect(bindings.oboeInit).throw(TypeError, 'invalid calling signature');
  })

  it('should init without losing memory', function (done) {
    // node 8, 10 completes in < 30 seconds but node 12 takes longer
    this.timeout(40000);
    const warmup = 1000000;
    const checkCount = 1000000;
    const options = Object.assign({}, goodOptions);

    // garbage collect if available
    const gc = typeof global.gc === 'function' ? global.gc : () => null;

    // allow the system to come to a steady state. garbage collection makes it
    // hard to isolate memory losses.
    const start1 = process.memoryUsage().rss;
    for (let i = warmup; i > 0; i--) {
      bindings.oboeInit(options);
    }

    gc();

    // now see if the code loses memory. if it's less than 1 byte per iteration
    // then it's not losing memory for all practical purposes.
    const start2 = process.memoryUsage().rss + checkCount;
    for (let i = checkCount; i > 0; i--) {
      bindings.oboeInit(options, {skipInit: true});
    }

    gc();

    // give garbage collection a window to kick in.
    setTimeout(function () {
      const finish = process.memoryUsage().rss;
      expect(finish).lte(start2, `should execute ${checkCount} metrics without memory growth`);
      //console.log('s1', start1, 's2', start2 - checkCount, 'fin', finish);
      done()
    }, 250)

    gc();
  })
})
