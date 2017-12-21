var bindings = require('../')

describe('addon.event', function () {
  var event

  bindings.oboeInit(process.env.APPOPTICS_SERVICE_KEY)

  it('should construct', function () {
    event = new bindings.Event()
  })

  it('should add info', function () {
    event.addInfo('key', 'val')
  })

  it('should add edge', function () {
    var e = new bindings.Event()
    var meta = e.getMetadata()
    event.addEdge(meta)
  })

  it('should add edge as string', function () {
    var e = new bindings.Event()
    var meta = e.toString()
    event.addEdge(meta.toString())
  })

  it('should get metadata', function () {
    var meta = event.getMetadata()
    meta.should.be.an.instanceof(bindings.Metadata)
  })

  it('should serialize metadata to id string', function () {
    var meta = event.toString()
    meta.should.be.an.instanceof(String).with.lengthOf(60)
    meta[0].should.equal('2')
    meta[1].should.equal('B')
  })

  it('should start tracing, returning a new instance', function () {
    var meta = new bindings.Metadata()
    var event = new bindings.Event(meta, false)
    //var event2 = bindings.Event.startTrace(meta)
  })
})
