/* global describe, before, it */
'use strict'

const bindings = require('../..')
const expect = require('chai').expect

const maxIsReadyToSampleWait = 60000

describe('reporter-metrics-memory', function () {
  const serviceKey = process.env.APPOPTICS_SERVICE_KEY || `${env.AO_TOKEN_NH}:node-bindings-test`
  const endpoint = process.env.APPOPTICS_COLLECTOR || 'collector-stg.appoptics.com'

  const metrics = []
  const batchSize = 100

  before(function () {
    this.timeout(maxIsReadyToSampleWait)
    const status = bindings.oboeInit({ serviceKey, endpoint })
    // oboeInit can return -1 for already initialized or 0 if succeeded.
    // depending on whether this is run as part of a suite or standalone
    // either result is valid.
    if (status !== -1 && status !== 0) {
      throw new Error('oboeInit() failed')
    }

    const start = Date.now()
    const ready = bindings.isReadyToSample(maxIsReadyToSampleWait)
    // eslint-disable-next-line no-console
    console.log(`[isReadyToSample() took ${Date.now() - start}ms]`)

    expect(ready).equal(1, `should be connected to ${endpoint} and ready`)

    for (let i = 0; i < batchSize; i++) {
      metrics.push({ name: 'node.metrics.batch.test', value: i })
    }
  })

  it('should sendMetric() without losing memory', function () {
    this.timeout(40000)
    const warmup = 500000
    const checkCount = 1000000
    // if it's less than 2 bytes per iteration it's good
    const margin = checkCount * 2

    // garbage collect if available
    const gc = typeof global.gc === 'function' ? global.gc : () => null

    /* eslint-disable no-unused-vars */
    let start1
    let done1
    let start2
    let done2
    let finish1
    /* eslint-enable no-unused-vars */

    // allow the system to come to a steady state. garbage collection makes it
    // hard to isolate memory losses.
    return wait()
      .then(function () {
        // eslint-disable-next-line no-unused-vars
        start1 = process.memoryUsage().rss
        for (let i = warmup; i > 0; i--) {
          bindings.Reporter.sendMetric('nothing.really', { value: i })
        }
        // eslint-disable-next-line no-unused-vars
        done1 = process.memoryUsage().rss
        gc(true)
      })
      .then(wait)
      .then(function () {
        // now see if the code loses memory. if it's less than 1 byte per iteration
        // then it's not losing memory for all practical purposes.
        start2 = process.memoryUsage().rss + checkCount
        for (let i = checkCount; i > 0; i--) {
          bindings.Reporter.sendMetric('nothing.really', { value: i, testing: true })
        }
        // eslint-disable-next-line no-unused-vars
        done2 = process.memoryUsage().rss
        gc(true)
      })
      .then(wait)
      .then(function () {
        finish1 = process.memoryUsage().rss
        // console.log(start1, done1, start2, done2, finish1);
        expect(finish1).lte(start2, `should execute ${checkCount} metrics with limited rss growth`)
        gc(true)
      })
      .then(wait)
      .then(function () {
        for (let i = checkCount; i > 0; i--) {
          bindings.Reporter.sendMetric('nothing.really', { value: i, testing: true })
        }
        gc(true)
      })
      .then(wait)
      .then(function () {
        const finish = process.memoryUsage().rss
        // console.log(start1, done1, start2, done2, finish);
        expect(finish).lte(finish1 + margin, 'rss should not show meaningful growth')
      })
  })

  it('should sendMetrics() without losing memory', function () {
    this.timeout(40000)
    const warmup = 500000
    const checkCount = 1000000
    // allowable margin
    const margin = checkCount * 1.5
    // garbage collect if available
    const gc = typeof global.gc === 'function' ? global.gc : () => null

    /* eslint-disable no-unused-vars */
    let start1
    let done1
    let start2
    let done2
    let finish1
    /* eslint-enable no-unused-vars */

    // allow the system to come to a steady state. garbage collection makes it
    // hard to isolate memory losses.
    return wait()
      .then(function () {
        // eslint-disable-next-line no-unused-vars
        start1 = process.memoryUsage().rss
        for (let i = warmup; i > 0; i -= batchSize) {
          bindings.Reporter.sendMetrics(metrics)
        }
        // eslint-disable-next-line no-unused-vars
        done1 = process.memoryUsage().rss
        gc(true)
      })
      .then(wait)
      .then(function () {
        // now see if the code loses memory. if it's less than 1 byte per iteration
        // then it's not losing memory for all practical purposes.
        start2 = process.memoryUsage().rss + checkCount
        for (let i = checkCount; i > 0; i -= batchSize) {
          bindings.Reporter.sendMetrics(metrics)
        }
        // eslint-disable-next-line no-unused-vars
        done2 = process.memoryUsage().rss
        gc(true)
      })
      .then(wait)
      .then(function () {
        finish1 = process.memoryUsage().rss
        // console.log(start1, done1, start2, done2, finish1);
        expect(finish1).lte(start2, `should execute ${checkCount} metrics with limited rss growth`)
        gc(true)
      })
      .then(wait)
      .then(function () {
        for (let i = checkCount; i > 0; i -= batchSize) {
          bindings.Reporter.sendMetrics(metrics)
        }
        gc(true)
      })
      .then(wait)
      .then(function () {
        const finish = process.memoryUsage().rss
        // console.log(start1, done1, start2, done2, finish);
        expect(finish).lte(finish1 + margin, 'rss should not show meaningful growth')
      })
  })
})

function wait (ms = 100) {
  return new Promise(resolve => {
    setTimeout(() => {
      resolve()
    }, ms)
  })
}
