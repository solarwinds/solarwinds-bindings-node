const pkg = require('../../package.json')

const patch = {
  name: '@appoptics/apm-bindings-dev',
  binary: {
    ...pkg.binary,
    staging_host: 'https://apm-appoptics-bindings-node-dev-staging.s3.amazonaws.com',
    production_host: 'https://apm-appoptics-bindings-node-dev-production.s3.amazonaws.com'
  }
}

// output new package.json for shell script to capture
console.log(JSON.stringify({ ...pkg, ...patch }, null, 2))
