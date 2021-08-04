/* global describe, it */
'use strict'

const bindings = require('../')
const expect = require('chai').expect

describe('addon.config', function () {
  it('should get oboe\'s version as a string', function () {
    const version = bindings.Config.getVersionString()
    expect(version).to.be.a('string')
    expect(version).match(/\d+\.\d+\.\d+/)
  })

  it('should get oboe\'s settings as an object', function () {
    const settings = bindings.Config.getSettings()
    expect(settings).to.be.a('object')
  })

  it('should get settings object with default values', function () {
    const settings = bindings.Config.getSettings()
    expect(Object.keys(settings).length).eq(4)
    expect(settings).property('tracing_mode', -1)
    expect(settings).property('sample_rate', -1)
    expect(settings).property('flag_sample_start', false)
    expect(settings).property('flag_through_always', false)
  })

  it('should get settings object as per spec', function () {
    const stats = bindings.Config.getSettings()
    expect(stats).to.have.all.keys(
      'tracing_mode',
      'sample_rate',
      'flag_sample_start',
      'flag_through_always'
    )
  })

  it('should get oboe\'s stats as object', function () {
    const stats = bindings.Config.getStats()
    expect(stats).to.be.a('object')
  })

  it('should get stats object as per spec', function () {
    const stats = bindings.Config.getStats()
    expect(Object.keys(stats).length).eq(5)
    expect(stats).to.have.all.keys(
      'reporterInitCount',
      'eventQueueFree',
      'collectorOk',
      'collectorTryLater',
      'collectorLimitExceeded'
    )
  })

  it('should get stats object with default values (no stats)', function () {
    const stats = bindings.Config.getStats()
    expect(stats).property('reporterInitCount', 0)
    expect(stats).property('eventQueueFree', 0)
    expect(stats).property('collectorOk', 0)
    expect(stats).property('collectorTryLater', 0)
    expect(stats).property('collectorTryLater', 0)
    expect(stats).property('collectorLimitExceeded', 0)
  })
})
