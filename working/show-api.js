'use strict';

const aob = require('..');

const keys = Object.keys(aob);

/* eslint-disable no-console */

keys.forEach(k => {
  if (typeof aob[k] === 'object') {
    // if it's a namespace show the functions
    console.log(k, Object.keys(aob[k]));
  } else if (typeof aob[k] === 'function' && aob[k].name === k) {
    // it's a constructor. display its static and instance methods.
    console.log(`[${k}]`, aob[k]);
    let props = staticProps(aob[k]);
    if (props.length) {
      console.log(`  static: ${props.join(', ')}`);
    }
    let o;
    if (k === 'Event') {
      o = new aob[k](aob.Metadata.makeRandom());
    } else {
      o = new aob[k]();
    }
    props = instanceProps(o);
    if (props.length) {
      console.log(`  instance: ${props.join(', ')}`);
    }
  } else {
    // it's a value or a simple function.
    console.log(k, aob[k]);
  }
})

function staticProps (o) {
  const allProps = new Set(Object.getOwnPropertyNames(o));
  const standardProps = new Set(Object.getOwnPropertyNames(Object.getPrototypeOf(o)));
  return [...allProps].filter(p => !standardProps.has(p)).filter(p => p !== 'prototype');
}

function instanceProps (o) {
  return Object.getOwnPropertyNames(Object.getPrototypeOf(o)).filter(p => p !== 'constructor');
}
