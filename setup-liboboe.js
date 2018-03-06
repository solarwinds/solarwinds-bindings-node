var spawn = require('child_process').spawnSync
var fs = require('fs')

var dir = './oboe/'

var chars = '⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'
var len = chars.length
function spinner(fps, fn) {
  var frame = 0

  var t = setInterval(function () {
    fn(chars[frame++ % len])
  }, 1000 / fps)

  return {
    stop: function () { clearInterval(t) }
  }
}


function setupLiboboe(cb) {
  var soname
  var childOutput = []
  var showOutput = process.env.APPOPTICS_SHOW_OBOE_SETUP

  var liboboeName
  fs.readdirSync(dir).forEach(function (f) {
    if (f.indexOf('liboboe') === 0 && !f.endsWith('.sha256')) {
      // if the file is a link delete it
      if (fs.lstatSync(dir + f).isSymbolicLink()) {
        fs.unlinkSync(dir + f)
      } else if (liboboeName) {
        console.warn('Multiple liboboes found:' + liboboeName + ", " + f)
      } else {
        liboboeName = f
      }
    }
  })

  if (!liboboeName) {
    console.warn('liboboe not found')
    process.exit(1)
  }

  var p = spawn('readelf', ['-d', './oboe/' + liboboeName])

  if (process.stdout.isTTY) {
    var spin = spinner(15, function (c) {
      process.stdout.clearLine()
      process.stdout.cursorTo(0)
      process.stdout.write(c + ' setting up liboboe for AppOptics native bindings')
    })
  }

  p.stderr.on('data', function (data) {
    if (showOutput) {
      childOutput.push(data)
    }
  })

  p.stdout.on('data', function(data) {
    var res = data.toString('utf8').match(/Library soname: \[(.+)]/)
    if (!res) {
      console.warn('liboboe missing SONAME')
    }
    // the first group captures the SONAME
    soname = res[1]
  })

  p.on('close', function (err) {
    if (process.stdout.isTTY) {
      spin.stop()
      process.stdout.clearLine()
      process.stdout.cursorTo(0)
    }

    if (err || !soname) {
      console.warn('AppOptics liboboe setup failed')
      if (showOutput) {
        childOutput = childOutput.join('').toString('utf8')
        console.warn(childOutput)
      }
    } else {
      fs.symlinkSync(liboboeName, dir + 'liboboe.so')
      fs.symlinkSync(liboboeName, dir + soname)

      console.log('AppOptics liboboe setup successful')
    }

    cb()
  })
}

if (!module.parent) {
  var i = setInterval(function() {}, 100)
  setupLiboboe(function () {clearInterval(i)})
} else {
  module.exports = setupLiboboe
}
