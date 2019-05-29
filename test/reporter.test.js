var bindings = require('../')
const expect = require('chai').expect

var realCollector = (!process.env.APPOPTICS_REPORTER
  || process.env.APPOPTICS_REPORTER === 'ssl')
  && (process.env.APPOPTICS_COLLECTOR === 'collector.appoptics.com'
  || process.env.APPOPTICS_COLLECTOR === 'collector-stg.appoptics.com')

describe('addon.reporter', function () {
  let r
  if (typeof bindings.Reporter === 'function') {
    r = new bindings.Reporter()
  } else {
    r = bindings.Reporter
  }
  const serviceKey = `${process.env.AO_TOKEN_PROD}:node-bindings-test`;

  beforeEach(function () {
    if (this.currentTest.title === 'should execute without losing memory') {

    }
  })


  it('should initialize oboe with only a service key', function () {
    const status = bindings.oboeInit({serviceKey})
    // kind of funky but -1 is already initialized, 0 is ok. mocha runs
    // multiple tests in one process so the result is 0 if run standalone
    // but -1 on all but the first if run as a suite.
    expect(status).oneOf([-1, 0]);
  })

  it('should initialize oboe with hostnameAlias', function () {
    const options = {serviceKey, hostnameAlias: 'node-testing-hostname'};
    const status = bindings.oboeInit(options);
    expect(status).equal(-1);
  })

  it('should check if ready to sample', function () {
    // wait 5 seconds max. This will fail if not using
    // a real collector (collector or collector-stg).appoptics.com
    var ready = r.isReadyToSample(5000)
    expect(ready).be.a('number')

    if (realCollector) {
      expect(ready).equal(1, `${process.env.APPOPTICS_COLLECTOR} should be ready`)
    } else {
      expect(ready).not.equal(1, 'isReadyToSample() should return 0')
    }
  })

  it('should send a generic span', function () {
    var customName = 'this-is-a-name'
    var domain = 'bruce.com'

    var finalTxName = r.sendNonHttpSpan({
      duration: 1001
    })
    expect(finalTxName).equal('unknown')

    finalTxName = r.sendNonHttpSpan({
      domain: domain
    })
    expect(finalTxName).equal(domain + '/')

    finalTxName = r.sendNonHttpSpan({
      txname: customName,
      duration: 1111
    })
    expect(finalTxName).equal(customName)

    finalTxName = r.sendNonHttpSpan({
      txname: customName,
      domain: domain,
      duration: 1234
    })
    expect(finalTxName).equal(domain + '/' + customName)
  })

  it('should send an HTTP span', function () {
    var customName = 'this-is-a-name'
    var domain = 'bruce.com'
    var url = '/api/todo'
    var status = 200
    var method = 'GET'

    var finalTxName = r.sendHttpSpan({
      url: url,
      status: status,
      method: method,
      duration: 1111
    })
    expect(finalTxName).equal(url)

    finalTxName = r.sendHttpSpan({
      url: url,
      domain: domain
    })
    expect(finalTxName).equal(domain + url)

    finalTxName = r.sendHttpSpan({
      txname: customName,
      url: url,
      duration: 1234
    })
    expect(finalTxName).equal(customName)

    finalTxName = r.sendHttpSpan({
      txname: customName,
      url: url,
      domain: domain,
      duration: 1236
    })
    expect(finalTxName).equal(domain + '/' + customName)
  })

  it('should not crash node getting the prototype of a reporter instance', function () {
    var p = Object.getPrototypeOf(r)
  })

  it('should throw errors for bad arguments to sendMetric', function () {
    const defaultOptions = {noop: true};

    const tests = [
      {args: [], text: 'no args'},
      {args: [1], text: 'non-string metric name'},
      {args: ['name', 'a'], text: 'non-object options'},
      {args: ['name', []], text: 'array-object options'},
      {args: ['name', {value: 'x'}], text: 'non-numeric value'},
      {args: ['name', {tags: []}], text: 'array-object tags'},
      {args: ['name', {tags: 11}], text: 'non-object tags'},
    ];

    for (let t of tests) {
      const options = Object.assign({}, defaultOptions, t.options)
      const fn = () => r.sendMetric(...t.args);
      expect(fn, `${t.text} should throw`).throws(TypeError);
    }

  })

  it('should send metrics without losing memory', function (done) {
    this.timeout(5000);
    const warmup =  1000000;
    const checkCount =  1000000;
    // garbage collect if available
    const gc = typeof global.gc === 'function' ? global.gc : () => null;

    // allow the system to come to a steady state. garbage collection makes it
    // hard to isolate memory losses.
    const start1 = process.memoryUsage().rss;
    for (let i = warmup; i > 0; i--) {
      r.sendMetric('nothing.really', {value: i, testing: true});
    }

    gc();

    // now see if the code loses memory. if it's less than 1 byte per iteration
    // then it's not losing memory for all practical purposes.
    const start2 = process.memoryUsage().rss + checkCount;
    for (let i = checkCount; i > 0; i--) {
      r.sendMetric('nothing.really', {value: i, testing: true});
    }

    gc();

    // give garbage collection a window to kick in.
    setTimeout(function () {
      const finish = process.memoryUsage().rss;
      expect(finish).lte(start2, `should execute ${checkCount} metrics without memory growth`);
      done()
    }, 250)

    if (typeof global.gc === 'function') {
      global.gc();
    }
  })
})
