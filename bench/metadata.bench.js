'use strict';
/* eslint-disable no-console */

const aob = require('..');
const Benchmark = require('benchmark');
const crypto = require('crypto');

const serviceKey = `${process.env.AO_TOKEN_PROD}:node-bench-metadata`;

const status = aob.oboeInit({serviceKey});
if (status > 0) {
  throw new Error('failed to initialize oboe');
}

// wait 2 seconds to make sure it's ready.
aob.Reporter.isReadyToSample(2000);

const suite = new Benchmark.Suite({name: 'crypto'});
let md1, md2, md3;

suite
  .add('oboe metadata', function () {
    md1 = aob.Metadata.makeRandom();
  })
  .add('oboe metadata', function () {
    md2 = aob.Metadata.makeRandom(1);
  })
  .add('crypto metadata', function () {
    md3 = Buffer.allocUnsafe(30);
    md3[0] = 0x2b;
    md3[29] = 0x00;
    crypto.randomFillSync(md3, 1, 28);
  })

  .on('complete', function () {
    console.log(this.name);
    for (let i = 0; i < this.length; i++) {
      const t = this[i];
      console.log(t.name, t.stats.mean, t.count, t.times.elapsed);
    }
    console.log(md1.toString(1));
    console.log(md2.toString(1));
    console.log(md3.toString('hex'));
  })

  .run();
