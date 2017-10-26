var bindings = require('../')

describe('addon.metadata', function () {
  var metadata
  var string
  bindings.oboeInit(process.env.APPOPTICS_SERVICE_KEY)

  it('should construct', function () {
    metadata = new bindings.Metadata()
  })

  it('should construct from random data', function () {
    metadata = bindings.Metadata.makeRandom()
    metadata.toString().should.not.equal('')
  })

  it('should serialize to string', function () {
    string = metadata.toString()
    string.should.not.equal('')
  })

  it('should construct from string', function () {
    metadata = bindings.Metadata.fromString(string)
    metadata.toString().should.equal(string)
  })

  it('should clone itself', function () {
    var rand = bindings.Metadata.makeRandom()
    rand.copy().toString().should.equal(rand.toString())
  })

  it('should be valid', function () {
    metadata.isValid().should.equal(true)
  })

  it('should create an event', function () {
    var event = metadata.createEvent()
    event.should.be.an.instanceof(bindings.Event)
  })
})
