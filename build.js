var spawn = require('child_process').spawn
var spawnSync = require('child_process').spawnSync

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
  var showOutput = process.env.APPOPTICS_SHOW_GYP
  var args = ['rebuild']
  if (process.env.APPOPTICS_BUILD_GYP_DEBUG) {
    args = ['--debug', 'configure'].concat(args)
  }
  var p = spawn('node-gyp', ['rebuild'])

  if (process.stdout.isTTY) {
    var spin = spinner(15, function (c) {
      process.stdout.clearLine()
      process.stdout.cursorTo(0)
      process.stdout.write(c + ' building AppOptics native bindings')
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
      console.warn('AppOptics bindings - failed to build, tracing disabled')
      if (showOutput) {
          childOutput = childOutput.join('').toString('utf8')
          console.warn(childOutput)
      }
    } else {
      console.log('AppOptics bindings built successfully')
    }

    cb()
  })
}

if ( ! module.parent) {
  var i = setInterval(function() {}, 100)
	build(function () {clearInterval(i)})
} else {
	module.exports = build
}
