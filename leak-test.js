'use strict'

const aob = require('.')

var startTime = new Date().getTime()
var initialMemUsage = process.memoryUsage()
var delayedMemUsage

// parameters
var interval = 10
var delayedBaseLine = minutes(0)

// optional data determined by specific test
var additional

function minutes (n) {
  return n * 60000
}

var arg = process.argv.slice(2)

var validTests = {
  'start-trace': startTrace,
  'make-random': makeRandom,
  'create-event-x-s': createEventX_S,
  'create-event-x-0': createEventX_0,
  'context-set-s': contextSet_S,
  'sample-trace-1': {fn: sampleTrace_1, setup: realSampleTraceSetup, interval: 100},
  'sample-trace-2': {fn: sampleTrace_2, setup: realSampleTraceSetup, interval: 100},
  'fake-sample': {fn: fakeSample, interval: 1},
}

if (!(process.argv[2] in validTests)) {
  console.log('bad test specified', process.argv[2])
  process.exit(1)
}

var metadata = new aob.Metadata.makeRandom()

var test = validTests[process.argv[2]]

if (typeof test === 'object') {
  let config = test
  test = config.fn
  if (config.setup) config.setup()
  if (config.interval) interval = config.interval
}

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

function realSampleTraceSetup () {
  additional = {sample: 0, not: 0}
}
function sampleTrace_1 () {
  var s = aob.Context.sampleTrace('leak-layer', '')
  additional[s.sample ? 'sample' : 'not'] += 1
}
function sampleTrace_2 () {
  var s = aob.Context.sampleTrace('leak-layer', metadata.toString())
  additional[s.sample ? 'sample' : 'not'] += 1
}


function fakeSample () {
  var s = aob.Context.fakeSample('leak-layer', '')
}



function outputCheck () {
  var now = new Date()
  var elapsed = elapsedTime((now.getTime() - startTime)/1000)
  // delay setting the baseline memory by delayedBaseLine to give it time
  // to settle after initial memory allocations.
  if (!delayedMemUsage && now.getTime() - timeBase >= delayedBaseLine) {
    delayedMemUsage = process.memoryUsage()
    initialMemUsage = delayedMemUsage
  }
  if (now.getTime() - timeBase > minutes(1)) {
    timeBase = now
    var currentMemUsage = process.memoryUsage()
    currentMemUsage.deltaRSS = currentMemUsage.rss - initialMemUsage.rss
    currentMemUsage.deltaHeap = currentMemUsage.heapUsed - initialMemUsage.heapUsed
    currentMemUsage.deltaRSSPerAlloc = currentMemUsage.deltaRSS / allocations
    currentMemUsage.deltaHeapPerAlloc = currentMemUsage.deltaHeap / allocations
    console.log(allocations, now, elapsed)
    console.log(currentMemUsage)
    if (additional) console.log(additional)
  }
}

function elapsedTime(seconds) {
  const h = Math.floor(seconds / 3600)
  const m = Math.floor((seconds % 3600) / 60)
  const s = Math.floor(seconds % 60)
  return [
    h,
    m > 9 ? m : (h ? '0' + m : m || '0'),
    s > 9 ? s : '0' + s,
  ].filter(a => a).join(':')
}
