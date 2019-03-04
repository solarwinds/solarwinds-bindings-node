'use strict'

const aob = require('..')

function minutes (n) {
  return n * 60000
}

var arg = process.argv.slice(2)

var validTests = {
  'start-trace': startTrace,
  'make-random': makeRandom,
  'create-event-x-0': createEventX_0,
  'create-event-x-s': createEventX_S,
  'create-event-x-m': createEventX_M,
  'create-event-x-e': createEventX_E,
  'context-set-s': contextSet_S,
  'sample-trace-1': {fn: sampleTrace_1, setup: realSampleTraceSetup, interval: 100},
  'sample-trace-2': {fn: sampleTrace_2, setup: realSampleTraceSetup, interval: 100},
  'fake-sample': {fn: fakeSample, interval: 1},
  'send-span-name': {fn: sendSpanName, setup: reporterSetup, interval: 10}
}

if (!(process.argv[2] in validTests)) {
  console.log('bad test specified', process.argv[2])
  process.exit(1)
}

var metadata = new aob.Metadata.makeRandom()
var event = new aob.Event(metadata)

// parameters
var interval = 10
var delayedBaseLine = minutes(0)

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

function makeRandom(n) {
  return Math.round(Math.random() * n)
}

var startTime = new Date().getTime()

// global memoryUsage objects
var allocations = 0
var initial = process.memoryUsage()
initial.allocations = 0
var current
var last = initial
var delayedMemUsage

console.log('starting', process.argv[2], 'pid', process.pid, '\n', initial)

// optional data determined by specific test
var additional
var iid = setInterval(harness, 10)
var timeBase = new Date().getTime()


//
// allocation test functions
//
function startTrace () {
  var e = aob.Context.startTrace()
}

function makeRandom () {
  var md = aob.Metadata.makeRandom()
}

//
// createEventX signatures
//
function createEventX_0 () {
  var e = aob.Context.createEventX()
}

function createEventX_S () {
  var e = aob.Context.createEventX(metadata.toString())
}

function createEventX_M() {
  var e = aob.Context.createEventX(metadata)
}

function createEventX_E() {
  var e = aob.Context.createEventX(event)
}

//
// Context.set
//
function contextSet_S () {
  aob.Context.set(metadata.toString())
}

//
//
//
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


//
// Inbound metrics reporting
//
var rep

function reporterSetup () {
  rep = new aob.Reporter()
}

function sendSpanName () {
  rep.sendHttpSpanName(
    'node-leak',
    makeRandom(20000),
    200,
    'GET',
    false
  )
}


const rd = (n, p) => n.toFixed(p !== undefined ? p : 2)
const delta = function (prev, field) {
  return current[field] - prev[field]
}
const deltaPer = function (prev, field) {
  return rd(delta(prev, field) / delta(prev, 'allocations'), 1)
}

function outputCheck () {
  var now = new Date()
  var elapsed = elapsedTime((now.getTime() - startTime)/1000)
  // delay setting the baseline memory by delayedBaseLine to give it time
  // to settle after initial memory allocations.
  if (!delayedMemUsage && now.getTime() - timeBase >= delayedBaseLine) {
    delayedMemUsage = process.memoryUsage()
    delayedMemUsage.allocations = allocations
    initial = delayedMemUsage
  }
  if (now.getTime() - timeBase > minutes(1)) {
    timeBase = now
    current = process.memoryUsage()
    current.allocations = allocations
    var eventInfo = aob.Event.getEventData()
    var metadataInfo = aob.Metadata.getMetadataData()
    var line1 = [
      'cur alloc: ', current.allocations,
      ', rss: ', current.rss,
      ', heapTot: ', current.heapTotal,
      ', heapUsed: ', current.heapUsed,
      ', x: ', current.external
    ].join('')
    var line2 = [
      'delta (initial) rss: ', delta(initial, 'rss'), ' (', deltaPer(initial, 'rss'), '/alloc)',
      ', heapTot: ', delta(initial, 'heapTotal'), ' (', deltaPer(initial, 'heapTotal'), '/alloc)',
      ', heapUsed: ', delta(initial, 'heapUsed'), ' (', deltaPer(initial, 'heapUsed'), '/alloc)',
    ].join('')
    var line3 = [
      'delta (last) rss: ', delta(last, 'rss'), ' (', deltaPer(last, 'rss'), '/alloc)',
      ', heapTot: ', delta(last, 'heapTotal'), ' (', deltaPer(last, 'heapTotal'), '/alloc)',
      ', heapUsed: ', delta(last, 'heapUsed'), ' (', deltaPer(last, 'heapUsed'), '/alloc)',
    ].join('')
    var line4 = [
      'events active: ', eventInfo.active,
      ', deleted: ', eventInfo.freedCount,
      ', deleted b: ', eventInfo.freedBytes,
      ', avg freed: ', rd(eventInfo.freedBytes / eventInfo.freedCount, 1)
    ].join('')
    var line5 = [
      'metadata active: ', metadataInfo.active,
      ', deleted: ', metadataInfo.freedCount,
      ', deleted b: ', metadataInfo.freedBytes,
    ].join('')
    console.log('et:', elapsed, '(' + now + ')')
    console.log([line1, line2, line3, line4, line5].join('\n'))
    if (additional) console.log(additional)
    last = current
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
