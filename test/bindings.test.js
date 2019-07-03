var bindings = require('../')
var expect = require('chai').expect;

const envOptions = {};
const prefixLen = 'APPOPTICS_'.length;

// portion of env var after 'APPOPTICS_': {bindings-name, type: [s]tring, [i]nteger, or [b]oolean}
const keyMap = {
  // these have been documented for end-users; the names should not be changed
  SERVICE_KEY: {name: 'serviceKey', type: 's'},
  TRUSTEDPATH: {name: 'trustedPath', type: 's'},
  HOSTNAME_ALIAS: {name: 'hostnameAlias', type: 's'},
  DEBUG_LEVEL: {name: 'logLevel', type: 'i'},

  // feel free to rationalize the following

  // used by node agent
  REPORTER: {name: 'reporter', type: 's'},
  COLLECTOR: {name: 'endpoint', type: 's'},
  TOKEN_BUCKET_CAPACITY: {name: 'tokenBucketCapacity', type: 'i'},      // file and udp reporter
  TOKEN_BUCKET_RATE: {name: 'tokenBucketRate', type: 'i'},              // file and udp reporter

  // not used by node agent
  BUFSIZE: {name: 'bufferSize', type: 'i'},
  LOGNAME: {name: 'logFilePath', type: 's'},
  TRACE_METRICS: {name: 'traceMetrics', type: 'b'},
  HISTOGRAM_PRECISION: {name: 'histogramPrecision', type: 'i'},
  MAX_TRANSACTIONS: {name: 'maxTransactions', type: 'i'},
  FLUSH_MAX_WAIT_TIME: {name: 'flushMaxWaitTime', type: 'i'},
  EVENTS_FLUSH_INTERVAL: {name: 'eventsFlushInterval', type: 'i'},
  EVENTS_FLUSH_BATCH_SIZE: {name: 'eventsFlushBatchSize', type: 'i'},
  REPORTER_FILE_SINGLE: {name: 'oneFilePerEvent', type: 'b'},           // file reporter
}

const goodOptions = {
  serviceKey: 'magical-service-key',
  trustedPath: 'secret-garden',
  hostnameAlias: 'incognito',
  logLevel: 100,
  reporter: 'udp',
  endpoint: 'localhost:port',
  tokenBucketCapacity: 100,
  tokenBucketRate: 100,
  bufferSize: 100,
  logFilePath: 'where-to-write',
  traceMetrics: true,
  histogramPrecision: 100,
  maxTransactions: 100,
  maxFlushWaitTime: 100,
  eventsFlushInterval: 100,
  eventsFlushBatchSize: 100,
  oneFilePerEvent: 1
}

const goodBooleans = {
  traceMetrics: true,
  oneFilePerEvent: true
}

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
  logFilePath: 0,
  traceMetrics: 0,
  histogramPrecision: 'a',
  maxTransactions: 'a',
  maxFlushWaitTime: 'a',
  eventsFlushInterval: 'a',
  eventsFlushBatchSize: 'a',
  oneFilePerEvent: undefined
}

const badBooleans = {
  traceMetrics: false,
  oneFilePerEvent: false
}

Object.keys(process.env).forEach(k => {
  if (!k.startsWith('APPOPTICS_')) {
    return;
  }
  const keyEntry = keyMap[k.slice(prefixLen)];
  if (!keyEntry) {
    return;
  }
  const value = convert(process.env[k], keyEntry.type);
  if (value !== undefined) {
    envOptions[keyEntry.name] = value;
  }
});

function convert (string, type) {
  if (type === 's') {
    return string;
  }
  if (type === 'i') {
    const v = +string;
    return Number.isNaN(v) ? undefined : v;
  }
  if (type === 'b') {
    if (['true', '1', 'yes', 'TRUE', 'YES'].indexOf(string) >= 0) {
      return true;
    }
    return false;
  }
  return undefined;
}

// set noop true so it doesn't actually call oboeInit().
const defaultOptions = {
  debug: true,
}


describe('addon.oboeInit()', function () {

  it('should handle good options values', function () {
    const options = Object.assign({}, goodOptions, defaultOptions);
    const expected = Object.assign({}, goodOptions, goodBooleans);
    var result = bindings.oboeInit(options);
    expect(result).deep.equal(expected);
  })

  it('should handle bad options values', function () {
    const options = Object.assign({}, badOptions, defaultOptions);
    var result = bindings.oboeInit(options);
    expect(result).deep.equal(badBooleans);
  })

  it('should handle the environment variable values', function () {
    const options = Object.assign({}, envOptions, defaultOptions)
    var result = bindings.oboeInit(options);
    expect(result).deep.equal(envOptions, 'initialization should succeeed');
  })

  it('should throw if not passed an object', function () {
    expect(bindings.oboeInit).throw(TypeError, 'invalid calling signature');
  })

  it('should init without losing memory', function (done) {
    // node 8, 10 completes in < 30 seconds but node 12 takes longer
    this.timeout(40000);
    const warmup = 1000000;
    const checkCount = 1000000;
    const options = Object.assign({}, goodOptions, defaultOptions);

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
      bindings.oboeInit(options);
    }

    gc();

    // give garbage collection a window to kick in.
    setTimeout(function () {
      const finish = process.memoryUsage().rss;
      expect(finish).lte(start2, `should execute ${checkCount} metrics without memory growth`);
      //console.log('s1', start1, 's2', start2 - checkCount, 'fin', finish);
      done()
    }, 250)


    if (typeof global.gc === 'function') {
      global.gc();
    }
  })
})
