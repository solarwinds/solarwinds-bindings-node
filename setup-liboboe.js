'use strict'

const fs = require('fs')
const releaseInfo = require('linux-os-info')

const dir = './oboe/'
const soname = 'liboboe-1.0.so.0'

// don't delete unused yet. old versions of npm (3.10.10) don't
// have prepare and prepublishOnly which are needed to sequence
// downloading/installing liboboe and the running setup-liboboe
// to get the links right.
const deleteUnused = false

//
// these versions of the .so library are included in the package.
//
const oboeNames = {
  linux: 'liboboe-1.0-x86_64.so.0.0.0',
  alpine: 'liboboe-1.0-alpine-x86_64.so.0.0.0'
}

function setupLiboboe (cb) {
  releaseInfo().then(info => {
    if (info.platform !== 'linux') {
      /* eslint-disable no-console */
      console.log(`warning: the ${info.platform} platform is not yet supported`)
      console.log(' ')
      /* eslint-enable no-console */
      process.exit(1)
    }
    let versionKey = 'linux'
    if (info.id === 'alpine') {
      versionKey = 'alpine'
    }
    return versionKey
  }).then(linux => {
    const liboboeName = oboeNames[linux]

    // must find the library
    if (!fs.existsSync(dir + liboboeName)) {
      throw new Error(`cannot find ${dir}${liboboeName}`)
    }

    //
    // setup the symbolic links to the right version of liboboe
    //

    // remove links if they exist
    if (isSymbolicLink(dir + 'liboboe.so')) {
      fs.unlinkSync(dir + 'liboboe.so')
    }

    if (isSymbolicLink(dir + soname)) {
      fs.unlinkSync(dir + soname)
    }

    fs.symlinkSync(liboboeName, dir + 'liboboe.so')
    fs.symlinkSync(liboboeName, dir + soname)

    // delete the unused file if indicated. ignore errors.
    if (deleteUnused) {
      const unusedOboe = oboeNames[linux === 'alpine' ? 'linux' : 'alpine']
      // do async so errors can easily be ignored
      fs.unlink(dir + unusedOboe, function (err) {
        console.log(err) // eslint-disable-line no-console
        process.exit(0)
      })
    }
    process.exit(0)
  }).catch(e => {
    console.error(e) // eslint-disable-line no-console
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
  const i = setInterval(function () {}, 100)
  setupLiboboe(function () { clearInterval(i) })
} else {
  module.exports = setupLiboboe
}
