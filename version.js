const fs = require('node:fs')

const pkg = JSON.parse(fs.readFileSync('package.json', 'utf8'))
const version = pkg.version

const prebuiltPackages = Object.keys(pkg.optionalDependencies)
  .filter((name) => name.startsWith('solarwinds-apm-bindings-'))

for (const p of prebuiltPackages) {
  pkg.optionalDependencies[p] = version
}

fs.writeFileSync('package.json', JSON.stringify(pkg, null, 2))
