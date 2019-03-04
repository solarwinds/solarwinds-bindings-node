var bindings = require('../')
var expect = require('chai').expect;

describe('addon.config', function () {

  it('should get oboe\'s version as a string', function () {
    var version = bindings.Config.getVersionString()
    expect(version).to.be.a('string')
    expect(version).match(/\d+\.\d+\.\d+/)
  })
})
