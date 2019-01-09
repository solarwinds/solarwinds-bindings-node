var bindings = require('../')
var expect = require('chai').expect;

describe('addon.config', function () {
  var version
  var revision

  it('should get oboe\'s version', function () {
    debugger
    version = bindings.Config.getVersion()
    expect(version).to.be.a('number')
  })

  it('should get oboe\'s revision', function () {
    revision = bindings.Config.getRevision()
    expect(revision).to.be.a('number')
  })

  it('should check valid oboe versions', function () {
    var check = bindings.Config.checkVersion(version, revision)
    expect(check).to.be.a('boolean')
    expect(check).equal(true)
  })

  it('should check invalid oboe versions', function () {
    var check = bindings.Config.checkVersion(10000, 0)
    expect(check).to.be.a('boolean')
    expect(check).equal(false)
  })

  it('should get oboe\'s version as a string', function () {
    var version = bindings.Config.getVersionString()
    expect(version).to.be.a('string')
    expect(version).match(/\d+\.\d+\.\d+/)
  })
})
