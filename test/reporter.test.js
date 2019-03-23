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

  beforeEach(function () {
    if (this.currentTest.title === 'should execute without losing memory') {

    }
  })

  it('should initialize oboe with hostnameAlias', function () {
    bindings.oboeInit(process.env.APPOPTICS_SERVICE_KEY, {
      hostnameAlias: 'node-testing-hostname'
    })
  })

  it('should initialize oboe with empty options', function () {
    bindings.oboeInit(process.env.APPOPTICS_SERVICE_KEY, {})
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

  it('should execute without losing memory', function (done) {
    this.timeout(5000);
    const warmup =  1000000;
    const check =  1000000;
    // allow the system to come to a steady state. garbage collection makes it
    // hard to isolate memory losses.
    const start1 = process.memoryUsage().rss;
    for (let i = warmup; i > 0; i--) {
      r.sendMetric('nothing.really', {value: i, testing: true});
    }

    // now see if the code loses memory.
    const start2 = process.memoryUsage().rss;
    for (let i = check; i > 0; i--) {
      r.sendMetric('nothing.really', {value: i, testing: true});
    }

    // give garbage collection a window to kick in.
    setTimeout(function () {
      const finish = process.memoryUsage().rss;
      expect(finish).lte(start2, `should execute ${check} metrics without memory growth`);
      done()
    }, 250)
  })
})
