'use strict'

var spawn = require('child_process').spawn
var rinfo = require('linux-release-info')
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

var soname = 'liboboe-1.0.so.0'


function setupLiboboe(cb) {
  var childOutput = []
  var showOutput = process.env.APPOPTICS_SHOW_OBOE_SETUP

  rinfo().then(info => {
    let basename = 'liboboe-1.0-'
    if (info.id === 'alpine') {
      basename += 'alpine-'
    }
    basename += 'x86_64.so.0.0.0'
    return basename
  }).then(liboboeName => {
    if (!fs.existsSync(dir + liboboeName)) {
      throw new Error('cannot find ' + dir + liboboeName)
    }
    // remove this link no matter what
    if (fs.lstatSync(dir + 'liboboe.so').isSymbolicLink()) {
      fs.unlinkSync(dir + 'liboboe.so')
    }
    // if the SONAME is hardcoded use it and avoid the readelf
    // dependency.
    if (soname) {
      if (fs.lstatSync(dir + soname).isSymbolicLink()) {
        fs.unlinkSync(dir + soname)
      }
      fs.symlinkSync(liboboeName, dir + 'liboboe.so')
      fs.symlinkSync(liboboeName, dir + soname)
      process.exit(0)
    }
    // the SONAME is not hardcoded so it must be read.
    return liboboeName
  }).then(liboboeName => {
    // if the SONAME can be dynamic it must be read from the .so file.
    var p = spawn('readelf', ['-d', dir + liboboeName])

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

    p.stdout.on('data', function (data) {
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
        if (fs.lstatSync(dir + soname).isSymbolicLink()) {
          fs.unlinkSync(dir + soname)
        }
        fs.symlinkSync(liboboeName, dir + 'liboboe.so')
        fs.symlinkSync(liboboeName, dir + soname)

        console.log('AppOptics liboboe setup successful')
      }

      cb()

      return 0
    })
  }).then(r => {
    process.exit(0)
  }).catch(e => {
    console.warn(e)
    process.exit(1)
  })

}

if (!module.parent) {
  var i = setInterval(function() {}, 100)
  setupLiboboe(function () {clearInterval(i)})
} else {
  module.exports = setupLiboboe
}
