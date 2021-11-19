/* global describe, before, it */
'use strict'

const net = require('net')
const fs = require('fs')
const util = require('util')
const EventEmitter = require('events')

const bindings = require('../..')
const expect = require('chai').expect

const maxIsReadyToSampleWait = 60000

const serviceKey = process.env.APPOPTICS_SERVICE_KEY || `${env.AO_TOKEN_NH}:node-bindings-test`
const endpoint = process.env.APPOPTICS_COLLECTOR || 'collector-stg.appoptics.com'

const notiSocket = '/tmp/ao-notifications'
let notiServer
let notiClient

const messages = []
const messageEmitter = new EventEmitter()

let messageConsumer = defaultMessageConsumer

function defaultMessageConsumer (msg) {
  messages.push(msg)
  messageEmitter.emit('message', msg)
}

describe('bindings.Notifier functions', function () {
  // remove the socket file if it exists.
  before(function (done) {
    fs.unlink(notiSocket, function () {
      done()
    })
  })

  // before calling oboe init setup the notification server
  before(function () {
    notiServer = net.createServer(function (client) {
      if (notiClient) {
        throw new Error('more than one client connection')
      }
      notiClient = client
      client.on('end', function () {
        notiClient = undefined
      })

      let previousData = ''
      client.on('data', function (data) {
        previousData = previousData + data.toString('utf8')
        let ix
        while ((ix = previousData.indexOf('\n')) >= 0) {
          const json = previousData.substring(0, ix)
          previousData = previousData.substring(ix + 1)
          const msg = JSON.parse(json)
          messageConsumer(msg)
        }
      })
    })

    notiServer.on('error', function (e) {
      throw e
    })
    notiServer.listen(notiSocket)
    notiServer.unref()
  })

  it('oboeNotifierInit() should work as expected', function (done) {
    const status = bindings.Notifier.init(notiSocket)
    expect(status).oneOf([0, -2], 'status must be OK or INITIALIZING')

    setTimeout(function () {
      if (messages.length) {
        const msg = messages.shift()
        expect(msg.seqNo).equal(0, 'first sequence number should be 0')
        expect(msg.source).equal('oboe', 'the source should be oboe')
        expect(msg.type).equal('keep-alive', 'the type should be keep-alive')
        done()
      } else {
        throw new Error('didn\'t get initial keep-alive message')
      }
    }, 500)
  })

  it('oboeInit() should successfully complete', function () {
    const details = {}
    const options = Object.assign({}, { serviceKey, endpoint })
    const expected = options
    const result = bindings.oboeInit(options, details)

    expect(result).lte(0, 'oboeInit() not get a version mismatch')
    expect(details.valid).deep.equal(expected)
    expect(details.processed).deep.equal(expected)

    const keysIn = Object.keys(options)
    const keysOut = Object.keys(details.processed)
    expect(keysIn.length).equal(keysOut.length)

    const ready = bindings.isReadyToSample(maxIsReadyToSampleWait)
    expect(ready).equal(1, `should be connected to ${endpoint} and ready`)
  })

  it(`should receive an oboe config message from ${endpoint}`, function (done) {
    const serviceName = serviceKey.slice(serviceKey.indexOf(':') + 1)
    const maskedKey = serviceKey.slice(0, 4) + '...' + serviceKey.slice(serviceKey.length - 4 - (':' + serviceName).length)
    let counter = 0
    const id = setInterval(function () {
      if (messages.length) {
        clearInterval(id)
        const msg = messages.shift()
        expect(msg.seqNo).equal(1, 'should have a seqNo of 1')
        expect(msg.source).equal('oboe', 'source didn\'t match')
        expect(msg.type).equal('config', 'oboe message should be type config')
        expect(msg.hostname).equal(endpoint, 'hostname should be what is set')
        expect(msg.port).equal(443, 'port should be 443')
        expect(msg.log).equal('', 'log should be empty')
        expect(msg.clientId).equal(maskedKey, 'service key should match')
        expect(msg.buffersize).equal(1000, 'buffersize should be 1000')
        expect(msg.maxTransactions).equal(200, 'maxTransactions should be 200')
        expect(msg.flushMaxWaitTime).equal(5000, 'flushMaxWaitTime should be 5000')
        expect(msg.eventsFlushInterval).equal(2, 'eventFlushInterval should be 2')
        expect(msg.maxRequestSizeBytes).equal(3000000, 'maxRequestSizeBytes should be 3e6')
        expect(msg.proxy).equal('', 'proxy should be empty')
        done()
      } else {
        counter += 1
        if (counter > 10) {
          clearInterval(id)
          throw new Error('no message arrived')
        }
      }
    }, 0)
  })

  it('should receive a collector remote-config message', function (done) {
    let counter = 0
    const id = setInterval(function () {
      if (messages.length) {
        clearInterval(id)
        const msg = messages.shift()
        expect(msg.seqNo).equal(2, 'should have a seqNo of 2')
        expect(msg.source).equal('collector')
        expect(msg.type).equal('remote-config', 'type should be remote-config')
        expect(msg).property('config', 'MetricsFlushInterval')
        expect(msg).property('value', 60)
        done()
      } else {
        counter += 1
        if (counter > 10) {
          clearInterval(id)
          throw new Error('no message arrived')
        }
      }
    }, 500)
  })

  it('should receive an oboe keep-alive message', function (done) {
    const version = bindings.Config.getVersionString()
    // if this is a custom build of oboe don't check this. x.x.1
    // sets a giant timeout so no keepalive messages will be sent.
    if ('0123456789'.indexOf(version[0]) < 0) {
      done()
      return
    }

    this.timeout(65000)
    let counter = 0
    const id = setInterval(function () {
      if (messages.length) {
        clearInterval(id)
        const msg = messages.shift()
        expect(msg.seqNo).equal(3, 'should have a seqNo of 3')
        expect(msg.source).equal('oboe')
        expect(msg.type).equal('keep-alive')
        done()
      } else {
        counter += 1
        if (counter > 10) {
          clearInterval(id)
          throw new Error('no message arrived')
        }
      }
    }, 10000)
  })

  it('should shutdown correctly using oboeNotifierStop()', function (done) {
    messageConsumer = msg => {
      throw new Error(`unexpected message arrived: ${util.format(msg)}`)
    }

    bindings.Notifier.stop()
    expect(bindings.Notifier.status()).oneOf([-1, -3], 'status should be disabled or shutting-down')

    setTimeout(function () {
      expect(bindings.Notifier.status()).equal(-1, 'status should be disabled')
      notiServer.getConnections((e, count) => {
        if (e) {
          throw e
        }
        expect(count).equal(0, 'server should have no connections')
        done()
      })
    }, 500)
  })

  it('oboeNotifierInit() should work as expected when re-initialized', function (done) {
    messageConsumer = defaultMessageConsumer
    messages.length = 0
    const status = bindings.Notifier.init(notiSocket)
    expect(status).oneOf([0, -2], 'status must be OK or INITIALIZING')

    setTimeout(function () {
      if (messages.length) {
        const msg = messages.shift()
        expect(msg.seqNo).equal(0, 'first sequence number should be 0')
        expect(msg.source).equal('oboe', 'the source should be oboe')
        expect(msg.type).equal('keep-alive', 'the type should be keep-alive')
        done()
      } else {
        throw new Error('didn\'t get initial keep-alive message')
      }
    }, 500)
  })

  it('should shutdown again using oboeNotifierStop()', function (done) {
    messageConsumer = msg => {
      throw new Error(`unexpected message arrived: ${util.format(msg)}`)
    }

    bindings.Notifier.stop()
    expect(bindings.Notifier.status()).oneOf([-3, -1], 'status should be shutting-down or disabled')

    setTimeout(function () {
      expect(bindings.Notifier.status()).equal(-1, 'status should be disabled')
      notiServer.getConnections((e, count) => {
        if (e) {
          throw e
        }
        expect(count).equal(0, 'server should have no connections')
        done()
      })
    }, 500)
  })
})
