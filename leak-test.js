const aob = require('.')

var initialMemUsage = process.memoryUsage()

var minutes = n => n * 60000

var arg = process.argv.slice(2)

var validTests = {
  'start-trace': startTrace,
  'make-random': makeRandom,
  'create-event-x-s': createEventX_S,
  'create-event-x-0': createEventX_0,
  'context-set-s': contextSet_S
}

if (!(process.argv[2] in validTests)) {
  console.log('bad test specified', process.argv[2])
  process.exit(1)
}

var metadata = new aob.Metadata.makeRandom()

var test = validTests[process.argv[2]]

function harness () {
  test()
  allocations += 1
  outputCheck()
}

console.log('starting', process.argv[2], 'pid', process.pid, '\n', initialMemUsage)
var iid = setInterval(harness, 10)
var allocations = 0
var timeBase = new Date().getTime()

function startTrace () {
  var e = aob.Context.startTrace()
}

function makeRandom () {
  var md = aob.Metadata.makeRandom()
}

function createEventX_0 () {
  var e = aob.Context.createEventX()
}

function createEventX_S () {
  var e = aob.Context.createEventX(metadata.toString())
}

function contextSet_S () {
  aob.Context.set(metadata.toString())
}



function outputCheck () {
  var now = new Date()
  if (now.getTime() - timeBase > minutes(1)) {
    timeBase = now
    currentMemUsage = process.memoryUsage()
    currentMemUsage.deltaRSS = currentMemUsage.rss - initialMemUsage.rss
    currentMemUsage.deltaHeap = currentMemUsage.heapUsed - initialMemUsage.heapUsed
    currentMemUsage.deltaRSSPerAlloc = currentMemUsage.deltaRSS / allocations
    currentMemUsage.deltaHeapPerAlloc = currentMemUsage.deltaHeap / allocations
    console.log(allocations, now)
    console.log(currentMemUsage)
  }
}
