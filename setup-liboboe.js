'use strict'

var spawn = require('child_process').spawn
var releaseInfo = require('linux-release-info')
var request = require('request')
var fs = require('fs')
var crypto = require('crypto')


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

const dir = './oboe/'
const soname = 'liboboe-1.0.so.0'
const downloadFile = false
// don't delete unused yet. old versions of npm (3.10.10) don't
// have prepare and prepublishOnly which are needed to sequence
// downloading/installing liboboe and the running setup-liboboe
// to get the links right.
const deleteUnused = false

//
// all files EXCEPT the .so file are packaged with each
// appoptics-bindings release. when this starts it reads
// the VERSION file and uses that to select which version
// of liboboe to download. within the version directory
// online it will choose either the alpine or standard
// file based on the contents of /etc/os-release. the
// .sha256 file is distributed as part of the release, not
// downloaded, to provide a double check that the right
// file is downloaded.
//

const oboeNames = {
  alpine: 'liboboe-1.0-alpine-x86_64.so.0.0.0',
  linux: 'liboboe-1.0-x86_64.so.0.0.0'
}

function setupLiboboe(cb) {
  var childOutput = []
  var showOutput = process.env.APPOPTICS_SHOW_OBOE_SETUP

  // get the version to read
  var version = fs.readFileSync(dir + 'VERSION', 'utf8').slice(0, -1)

  // find out whether it's alpine or not
  releaseInfo().then(info => {
    return info.id === 'alpine' ? 'alpine' : 'linux'
  }).then(linux => {
    const liboboeName = oboeNames[linux]
    //
    // if not downloading just use the file that is there. this
    // requires that all target system versions of liboboe are
    // distributed as part of the npm package.
    //
    if (!downloadFile) {
      if (!fs.existsSync(dir + liboboeName)) {
        throw new Error('cannot find ' + dir + liboboeName)
      }
      return linux
    }
    //
    // read the expected hash value. let any error propagate and
    // terminate the promise chain.
    //
    const hashFile = './oboe/' + liboboeName + '.sha256'
    const expectedHash = fs.readFileSync(hashFile, 'utf8').slice(0, -1)

    // set up a stream to calculate the hash.
    const hash = crypto.createHash('sha256').setEncoding('hex')

    const target = 'https://files.appoptics.com/c-lib/' + version + '/' + liboboeName
    console.log('downloading', target)
    //
    // download the correct file and check the .sha256
    //
    return new Promise(function (resolve, reject) {
      request.get(target)
        // handle errors
        .on('error', function (err) {reject(err)})
        // calculate the sha256 hash
        .on('data', chunk => hash.write(chunk))
        // write the file
        .pipe(fs.createWriteStream(dir + 'xyzzy'))
        // notice when it's done, check the hash, and handle
        // appropriately.
        .on('finish', function () {
          hash.end()
          let hashFound = hash.read()
          if (hashFound !== expectedHash && !process.env.AO_IGNORE_SHA256) {
            reject(new Error('invalid hash: ' + hashFound))
          }
          // pass the filename to the next step
          resolve(linux)
        })
    })

  }).then(linux => {
    const liboboeName = oboeNames[linux]
    //
    // here setup the symbolic links to the right version of liboboe
    //

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
      // delete the unused file. ignore errors.
      if (deleteUnused) {
        let unusedOboe = oboeNames[linux === 'alpine' ? 'linux' : 'alpine']
        // do async so errors can easily be ignored
        fs.unlink(dir + unusedOboe, function (err) {
          console.log(err)
          process.exit(0)
        })
      }
      process.exit(0)
    }
    //
    // if here then the SONAME is not hardcoded so it must be read
    // from liboboe.
    //
    return linux
  }).then(linux => {
    const liboboeName = oboeNames[linux]
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

      return linux
    })
  }).then(linux => {
    if (deleteUnused) {
      // delete the unused file. ignore errors.
      let unusedOboe = oboeNames[linux === 'alpine' ? 'linux' : 'alpine']
      fs.unlink(dir + unusedOboe, function () {
        process.exit(0)
      })
    }
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
