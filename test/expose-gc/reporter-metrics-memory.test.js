const aob = require('../..');
const r = aob.Reporter;
const expect = require('chai').expect;

const env = process.env;

describe('reporter-metrics-memory', function () {
  const serviceKey = `${env.AO_TOKEN_PROD}:node-bindings-test`;
  const metrics = [];
  const batchSize = 100;

  before(function () {
    const status = aob.oboeInit({serviceKey});
    // oboeInit can return -1 for already initialized or 0 if succeeded.
    // depending on whether this is run as part of a suite or standalone
    // either result is valid.
    if (status !== -1 && status !== 0) {
      throw new Error('oboeInit() failed');
    }

    const ready = aob.isReadyToSample(5000);
    expect(ready).equal(1, `should connected to ${env.APPOPTICS_COLLECTOR} and ready`);

    for (let i = 0; i < batchSize; i++) {
      metrics.push({name: 'node.metrics.batch.test', value: i});
    }
  });

  it('should sendMetric() without losing memory', function () {
    this.timeout(40000);
    const warmup =  500000;
    const checkCount =  1000000;
    // if it's less than 1 byte per iteration it's good
    const margin = checkCount;
    // garbage collect if available
    const gc = typeof global.gc === 'function' ? global.gc : () => null;

    /* eslint-disable no-unused-vars */
    let start1;
    let done1;
    let start2;
    let done2;
    let finish1;
    /* eslint-enable no-unused-vars */

    // allow the system to come to a steady state. garbage collection makes it
    // hard to isolate memory losses.
    return wait()
      .then(function () {
        start1 = process.memoryUsage().rss;
        for (let i = warmup; i > 0; i--) {
          r.sendMetric('nothing.really', {value: i});
        }
        done1 = process.memoryUsage().rss;
        gc(true);
      })
      .then(wait)
      .then(function () {
        // now see if the code loses memory. if it's less than 1 byte per iteration
        // then it's not losing memory for all practical purposes.
        start2 = process.memoryUsage().rss + checkCount;
        for (let i = checkCount; i > 0; i--) {
          r.sendMetric('nothing.really', {value: i, testing: true});
        }
        done2 = process.memoryUsage().rss;
        gc(true);
      })
      .then(wait)
      .then(function () {
        finish1 = process.memoryUsage().rss;
        //console.log(start1, done1, start2, done2, finish1);
        expect(finish1).lte(start2, `should execute ${checkCount} metrics with limited rss growth`);
        gc(true);
      })
      .then(wait)
      .then(function () {
        for (let i = checkCount; i > 0; i--) {
          r.sendMetric('nothing.really', {value: i, testing: true});
        }
        gc(true);
      })
      .then(wait)
      .then(function () {
        const finish = process.memoryUsage().rss;
        //console.log(start1, done1, start2, done2, finish);
        expect(finish).lte(finish1 + margin, 'rss should not show meaningful growth');
      })
  })

  it('should sendMetrics() without losing memory', function () {
    this.timeout(40000);
    const warmup = 500000;
    const checkCount = 1000000;
    // allowable margin
    const margin = checkCount;
    // garbage collect if available
    const gc = typeof global.gc === 'function' ? global.gc : () => null;

    /* eslint-disable no-unused-vars */
    let start1;
    let done1;
    let start2;
    let done2;
    let finish1;
    /* eslint-enable no-unused-vars */

    // allow the system to come to a steady state. garbage collection makes it
    // hard to isolate memory losses.
    return wait()
      .then(function () {
        start1 = process.memoryUsage().rss;
        for (let i = warmup; i > 0; i -= batchSize) {
          r.sendMetrics(metrics);
        }
        done1 = process.memoryUsage().rss;
        gc(true);
      })
      .then(wait)
      .then(function () {
        // now see if the code loses memory. if it's less than 1 byte per iteration
        // then it's not losing memory for all practical purposes.
        start2 = process.memoryUsage().rss + checkCount;
        for (let i = checkCount; i > 0; i -= batchSize) {
          r.sendMetrics(metrics);
        }
        done2 = process.memoryUsage().rss;
        gc(true);
      })
      .then(wait)
      .then(function () {
        finish1 = process.memoryUsage().rss;
        //console.log(start1, done1, start2, done2, finish1);
        expect(finish1).lte(start2, `should execute ${checkCount} metrics with limited rss growth`);
        gc(true);
      })
      .then(wait)
      .then(function () {
        for (let i = checkCount; i > 0; i -= batchSize) {
          r.sendMetrics(metrics);
        }
        gc(true);
      })
      .then(wait)
      .then(function () {
        const finish = process.memoryUsage().rss;
        //console.log(start1, done1, start2, done2, finish);
        expect(finish).lte(finish1 + margin, 'rss should not show meaningful growth');
      })
  })
})

function wait (ms = 100) {
  return new Promise(resolve => {
    setTimeout(() => {
      resolve();
    }, ms);
  });
}
