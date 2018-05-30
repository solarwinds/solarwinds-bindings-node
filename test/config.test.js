var bindings = require('../')

describe('addon.config', function () {
  var version
  var revision

  it('should get oboe\'s version', function () {
    version = bindings.Config.getVersion()
    version.should.be.an.instanceof(Number)
  })

  it('should get oboe\'s revision', function () {
    revision = bindings.Config.getRevision()
    revision.should.be.an.instanceof(Number)
  })

  it('should check valid oboe versions', function () {
    var check = bindings.Config.checkVersion(version, revision)
    check.should.be.an.instanceof(Boolean)
    check.should.equal(true)
  })

  it('should check invalid oboe versions', function () {
    var check = bindings.Config.checkVersion(10000, 0)
    check.should.be.an.instanceof(Boolean)
    check.should.equal(false)
  })

  it('should get oboe\'s version as a string', function () {
    var version = bindings.Config.getVersionString()
    version.should.be.an.instanceof(String)
    version.should.match(/\d+\.\d+\.\d+/)
  })
})
