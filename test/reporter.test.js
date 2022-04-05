/* global describe, before, it */
'use strict'

const bindings = require('../')
const expect = require('chai').expect

const maxIsReadyToSampleWait = 60000

describe('bindings.Reporter', function () {
  before(function () {
    const serviceKey = process.env.SW_APM_SERVICE_KEY
    const endpoint = process.env.SW_APM_COLLECTOR

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

  let r
  if (typeof bindings.Reporter === 'function') {
    r = new bindings.Reporter()
  } else {
    r = bindings.Reporter
  }

  // TODO: actually test hostnameAlias. Move elsewhere?
  // it('should initialize oboe with hostnameAlias', function () {
  //   const status = bindings.oboeInit({ hostnameAlias: 'node-testing-hostname' })
  //   // kind of funky but -1 is already initialized, 0 is ok. mocha runs
  //   // multiple tests in one process so the result is 0 if run standalone
  //   // but -1 on all but the first if run as a suite.
  //   expect(status).oneOf([-1, 0])
  // })

  it('should send a generic span', function () {
    const customName = 'this-is-a-name'
    const domain = 'bruce.com'

    let finalTxName = r.sendNonHttpSpan({
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
    const customName = 'this-is-a-name'
    const domain = 'bruce.com'
    const url = '/api/todo'
    const status = 200
    const method = 'GET'

    let finalTxName = r.sendHttpSpan({
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
    // eslint-disable-next-line no-unused-vars
    const p = Object.getPrototypeOf(r)
  })
})
