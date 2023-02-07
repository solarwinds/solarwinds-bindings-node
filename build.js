const fs = require('node:fs/promises')
const path = require('node:path')
const { build } = require('zig-build')

const nan = path.dirname(require.resolve('nan'))
const version = Number.parseInt(process.version.split('.')[0].slice(1))
const engines = { node: `>=${version} <${version + 1}` }

/** @type{import('zig-build').Target} */
const bindings = {
  std: 'c++14',
  napiVersion: 6,
  exceptions: true,
  sources: [
    'src/bindings.cc',
    'src/settings.cc',
    'src/config.cc',
    'src/event.cc',
    'src/event/event-to-string.cc',
    'src/event/event-send.cc',
    'src/reporter.cc'
  ],
  include: [__dirname, 'src'],
  libraries: ['oboe'],
  rpath: '$ORIGIN',
  cflags: ['-Wall', '-Wextra']
}

/** @type{import('zig-build').Target} */
const metrics = {
  std: 'c++14',
  sources: [
    'src/metrics/metrics.cc',
    'src/metrics/gc.cc',
    'src/metrics/eventloop.cc',
    'src/metrics/process.cc'
  ],
  libraries: ['oboe'],
  include: [__dirname, nan],
  rpath: '$ORIGIN',
  cflags: ['-Wall', '-Wextra']
}

const targets = [
  {
    name: `x64-linux-gnu-${version}`,
    target: 'x86_64-linux-gnu',
    glibc: '2.27',
    cpu: 'sandybridge',
    oboe: 'liboboe-1.0-x86_64.so',
    npm: {
      cpu: ['x64'],
      os: ['linux'],
      libc: ['glibc'],
      engines
    }
  },
  {
    name: `x64-linux-musl-${version}`,
    target: 'x86_64-linux-musl',
    cpu: 'sandybridge',
    oboe: 'liboboe-1.0-alpine-x86_64.so',
    npm: {
      cpu: ['x64'],
      os: ['linux'],
      libc: ['musl'],
      engines
    }
  },
  {
    name: `arm64-linux-gnu-${version}`,
    target: 'aarch64-linux-gnu',
    glibc: '2.27',
    cpu: 'baseline',
    oboe: 'liboboe-1.0-aarch64.so',
    npm: {
      cpu: ['arm64'],
      os: ['linux'],
      libc: ['glibc'],
      engines
    }
  },
  {
    name: `arm64-linux-musl-${version}`,
    target: 'aarch64-linux-musl',
    cpu: 'baseline',
    oboe: 'liboboe-1.0-alpine-aarch64.so',
    npm: {
      cpu: ['arm64'],
      os: ['linux'],
      libc: ['musl'],
      engines
    }
  }
]

const bindingsTargets = Object.fromEntries(targets.map((target) => [
    `bindings-${target.name}`,
    {
      ...bindings,
      target: target.target,
      glibc: target.glibc,
      cpu: target.cpu,
      output: `npm/${target.name}/bindings.node`,
      librariesSearch: [`npm/${target.name}`]
    }
]))

const metricsTargets = Object.fromEntries(targets.map((target) => [
    `metrics-${target.name}`,
    {
      ...metrics,
      target: target.target,
      glibc: target.glibc,
      cpu: target.cpu,
      output: `npm/${target.name}/metrics.node`,
      librariesSearch: [`npm/${target.name}`]
    }
]))

async function prepare () {
  const pkg = JSON.parse(await fs.readFile('package.json', 'utf8'))

  await Promise.all(targets.map(async (target) => {
    const targetPkg = {
      name: `${pkg.name}-${target.name}`,
      version: pkg.version,
      license: pkg.license,
      contributors: pkg.contributors,
      ...target.npm
    }
    const targetDir = path.join('npm', target.name)

    await fs.rm(targetDir, { recursive: true, force: true })
    await fs.mkdir(targetDir, { recursive: true })

    await fs.writeFile(
      `${targetDir}/package.json`,
      JSON.stringify(targetPkg, null, 2)
    )
    await fs.copyFile(`oboe/${target.oboe}`, `${targetDir}/liboboe.so`)
  }))
}

prepare()
  .then(() => build({ ...bindingsTargets, ...metricsTargets }))
  .catch((err) => console.error(err))
