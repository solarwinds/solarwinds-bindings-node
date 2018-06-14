'use strict'

var spawn = require('child_process').spawn
var releaseInfo = require('linux-release-info')
var request = require('request')
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

//
// all files EXCEPT the .so file (and .sha256?) are
// packaged with each appoptics-bindings release. when
// this starts it reads the VERSION file to determine
// which version of liboboe to read. within that version
// directory it will choose either the alpine version or
// the standard version based on the architecture of the
// system it is running on.
//

function setupLiboboe(cb) {
  var childOutput = []
  var showOutput = process.env.APPOPTICS_SHOW_OBOE_SETUP

  var version = fs.readFileSync('./oboe/VERSION', 'utf8')
  if (version[version.length - 1] === '\n') {
    version = version.slice(0, -1)
  }

  releaseInfo().then(info => {
    let basename = 'liboboe-1.0-'
    if (info.id === 'alpine') {
      basename += 'alpine-'
    }
    basename += 'x86_64.so.0.0.0'
    return basename
  }).then(liboboeName => {
    const target = 'https://files.appoptics.com/c-lib/' + version + '/' + liboboeName
    console.log('will download', target)
    // now download the correct file
    // read correct version from VERSION
    return new Promise(function (resolve, reject) {
      request.get(target)
        // handle errors
        .on('error', function (err) {reject(err)})
        // write the file
        .pipe(fs.createWriteStream('./oboe/xyzzy'))
        // notice when it's done and return the filename
        .on('finish', function () {resolve(liboboeName)})
    })

  }).then(liboboeName => {
    if (!fs.existsSync(dir + liboboeName)) {
      throw new Error('cannot find ' + dir + liboboeName)
    }
    // remove this link no matter what
    if (isSymbolicLink(dir + 'liboboe.so')) {
      fs.unlinkSync(dir + 'liboboe.so')
    }
    // if the SONAME is hardcoded use it and avoid the readelf
    // dependency.
    if (soname) {
      if (isSymbolicLink(dir + soname)) {
        fs.unlinkSync(dir + soname)
      }
      fs.symlinkSync(liboboeName, dir + 'liboboe.so')
      fs.symlinkSync(liboboeName, dir + soname)
      process.exit(0)
    }
    //
    // if here then the SONAME is not hardcoded so it must be read
    // from liboboe.
    //
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
        if (isSymbolicLink(dir + soname)) {
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

function isSymbolicLink (name) {
  try {
    if (fs.lstatSync(name).isSymbolicLink()) {
      return true
    }
  } catch (e) {
    // ignore errors
  }
  return false
}

if (!module.parent) {
  var i = setInterval(function() {}, 100)
  setupLiboboe(function () {clearInterval(i)})
} else {
  module.exports = setupLiboboe
}
