var spawn = require('child_process').spawn

var chars = '⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'
var len = chars.length
function spinner (fps, fn) {
  var frame = 0

  var t = setInterval(function () {
    fn(chars[frame++ % len])
  }, 1000 / fps)

  return {
    stop: function () { clearInterval(t) }
  }
}


function build (cb) {
    var childOutput = []
    var showOutput = false

    var p = spawn('node-gyp', ['rebuild'])

  if (process.stdout.isTTY) {
    var spin = spinner(15, function (c) {
      process.stdout.clearLine()
      process.stdout.cursorTo(0)
      process.stdout.write(c + ' building TraceView native bindings')
    })
  }

  p.stderr.on('data', function (data) {
      if (showOutput) {
          childOutput.push(data)
      }
  })

  p.on('close', function (err) {
    if (process.stdout.isTTY) {
      spin.stop()
      process.stdout.clearLine()
      process.stdout.cursorTo(0)
    }

    if (err) {
      console.warn('TraceView oboe library not found, tracing disabled')
      if (showOutput) {
          childOutput = childOutput.join('').toString('utf8')
          console.warn(childOutput)
      }
    } else {
      console.log('TraceView bindings built successfully')
    }

    cb()
  })
}

if ( ! module.parent) {
	build(function () {})
} else {
	module.exports = build
}
