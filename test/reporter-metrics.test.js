'use strict';

const aob = require('../');
const expect = require('chai').expect;

const env = process.env;

describe('aob.Reporter.sendMetrics()', function () {
  const serviceKey = `${env.AO_TOKEN_PROD}:node-bindings-test`;

  before(function () {
    const status = aob.oboeInit({serviceKey});
    // oboeInit can return -1 for already initialized or 0 if succeeded.
    // depending on whether this is run as part of a suite or standalone
    // either result is valid.
    if (status !== -1 && status !== 0) {
      throw new Error('oboeInit() failed');
    }

    const ready = aob.Reporter.isReadyToSample(5000);
    expect(ready).equal(1, `should connected to ${env.APPOPTICS_COLLECTOR} and ready`);
  })

  it('should handle an array of zero length', function () {
    const status = aob.Reporter.sendMetrics([]);
    expect(status).deep.equal({errors: []});
  })

  it('should require an array argument', function () {
    const args = [
      {i: 'am not an array'},
      'nor am i',
      0,
      false,
      Symbol('xyzzy'),
    ];

    for (let i = 0; i < args.length; i++) {
      function test () {
        aob.Reporter.sendMetrics(args[i]);
      }
      expect(test).throws('invalid signature for sendMetrics()');
    }
  })

  it('should return bad metrics', function () {
    const x = Symbol('expected-error');
    const metrics = [
      {count: 1, [x]: 2},
      {name: 'gc.count', count: 'x', [x]: 3},
      {name: 'gc.count', count: -1, [x]: 4},
      {name: 'gc.count', count: 1, value: 'x', [x]: 5},
      {name: 'gc.count', count: 1, value: 3.141592654, tags: 'i am not an object', [x]: 6},
    ];

    const results = aob.Reporter.sendMetrics(metrics);
    expect(results).not.property('correct');
    expect(results).property('errors').an('array');
    expect(results.errors.length).equal(metrics.length);

    for (let i = 0; i < metrics.length; i++) {
      const error = results.errors[i];
      const expectedError = error.metric[x];
      delete error.metric[x];
      expect(error.code).equal(expectedError);
      expect(error.metric).deep.equal(metrics[i]);
    }
  });

  it('should handle correct metrics', function () {
    const metrics = [
      {name: 'testing.node.abc', count: 1},
      {name: 'testing.node.def', count: 1, value: 42},
      {name: 'testing.node.xyz', count: 1, value: 84, addHostTag: true},
      {name: 'testing.node.xyz', count: 1, value: 2.71828, addHostTag: false},
      {name: 'testing.node.gc', count: 4, tags: {bruce: 'test', macnaughton: 'name'}},
      {name: 'testing.node.bool', count: 1, addHostTag: 1},
      {name: 'testing.node.bool', count: 1, addHostTag: ''},
    ];
    const results = aob.Reporter.sendMetrics(metrics, {testing: true});
    expect(results).property('correct').an('array');
    expect(results.correct.length).equal(metrics.length);
    expect(results).property('errors').an('array');
    expect(results.errors.length).equal(0);

    for (let i = 0; i < results.correct.length; i++) {
      const metric = results.correct[i];
      const expected = Object.assign({}, metrics[i], {addHostTag: !!metrics[i].addHostTag});
      expect(metric).deep.equal(expected);
    }
  });

  it.skip('should send metrics without losing memory', function (done) {
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

    gc();
  })
})
