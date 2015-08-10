var bindings = require('../')

describe('addon.config', function () {
  var version
  var revision

  it('should get version', function () {
    version = bindings.Config.getVersion()
    version.should.be.an.instanceof(Number)
  })

  it('should get revision', function () {
    revision = bindings.Config.getRevision()
    revision.should.be.an.instanceof(Number)
  })

  it('should check valid versions', function () {
    var check = bindings.Config.checkVersion(version, revision)
    check.should.be.an.instanceof(Boolean)
    check.should.equal(true)
  })

  it('should check invalid versions', function () {
    var check = bindings.Config.checkVersion(10000, 0)
    check.should.be.an.instanceof(Boolean)
    check.should.equal(false)
  })
})
