'use strict';

const bindings = require('../');
const aob = bindings;
const expect = require('chai').expect

const evUnsampled = '2B5BD5777CA0077C734B537B64C6B969213358B9F21AA0B1B42979F5C400';
const evSampled = '2B4FC9017BA3404828F253638A697DC7CF1A6BB1A4A544D5B98159B55501';

describe('addon.event', function () {
  const serviceKey = `${process.env.AO_TOKEN_PROD}:node-bindings-test`;

  it('should initialize oboe with only a service key', function () {
    const status = bindings.oboeInit({serviceKey})
    // kind of funky but -1 is already initialized, 0 is ok. mocha runs
    // multiple tests in one process so the result is 0 if run standalone
    // but -1 on all but the first if run as a suite.
    expect(status).oneOf([-1, 0]);
  })

  it('should throw if the constructor is called without "new"', function () {
    function badCall () {
      bindings.Event();
    }
    expect(badCall, 'should throw without "new"').throws(TypeError, 'Class constructors cannot be invoked without \'new\'');
  })

  it('should succeed when called with no arguments', function () {
    const event = new aob.Event();
    expect(event).instanceof(aob.Event);
  });

  it('should throw if the constructor is called without an event', function () {
    const tests = [
      {args: [1], text: 'non-metadata', e: 'invalid signature'},
      {args: ['invalid-x-trace-string'], text: 'invalid x-trace', e: 'invalid signature'},
    ];

    for (const t of tests) {
      function badCall () {
        new bindings.Event(...t.args);
      }
      expect(badCall, `${t.text} should throw "${t.e}"`).throws(TypeError, t.e);
    }
  })

  it('should construct an event using an event', function () {
    const fmt = 1|2|8;        // header, task, flags
    const eventTemplate = aob.Event.makeRandom();
    const event = new bindings.Event(eventTemplate);
    expect(event.toString(fmt)).equal(eventTemplate.toString(fmt));
  })

  it('should construct an event with or without an edge', function () {
    const fmt = 1|2|8;        // just compare the header, task, and flags.
    const event = new aob.Event.makeRandom();

    const tests = [
      {args: [event, false], text: 'no edge'},
      {args: [event, true], text: 'edge'},
    ];

    for (const t of tests) {
      const ev = new aob.Event(...t.args);
      expect(ev.toString(fmt), `with "${t.text}"`).equal(event.toString(fmt));
    }
  })

  it('should construct an event using metadata', function () {
    const random = aob.Event.makeRandom();
    const headerAndTaskId = random.toString().slice(0, 42);
    const event = new aob.Event(random);
    // They should share the same task ID'
    expect(event.toString().slice(0, 42)).equal(headerAndTaskId)
    // they should have different op IDs.
    expect(event.toString().slice(42, 58)).not.equal(random.toString().slice(42, 58))
    expect(event.getSampleFlag()).equal(false)
  })

  it('should construct an event with the sample flag set', function () {
    const ev1 = aob.Event.makeRandom(1);
    const ev2 = new aob.Event(ev1);
    expect(ev1.toString().slice(-2)).equal('01');
    expect(ev2.toString().slice(-2)).equal('01');
    expect(ev1.getSampleFlag()).equal(true);
    expect(ev2.getSampleFlag()).equal(true);
    expect(ev2.toString(1|2|8)).equal(ev1.toString(1|2|8));

  })

  it('should add a KV using addInfo', function () {
    const event = new aob.Event(aob.Event.makeRandom());
    event.addInfo('key', 'val')
  })

  it('should add edge from an event', function () {
    const event = new aob.Event(aob.Event.makeRandom());
    const edge = new aob.Event(event);
    event.addEdge(edge);
  })

  it('should add edge from a string', function () {
    const event = new aob.Event(aob.Event.makeRandom());
    const edge = new bindings.Event(event).toString();
    event.addEdge(edge);
  })

  it('should not add edge when task IDs do not match', function () {
    const event = new aob.Event(aob.Event.makeRandom());
    const edge = aob.Event.makeRandom();
    expect(edge.toString(2)).not.equal(event.toString(2));

    // force event to a different task ID because oboe returns "OK"
    // when the event metadata is invalid. And just calling
    // new Event() results in 2B0000000..0000<op id><flags>, i.e.,
    // invalid metadata. So it doesn't fail even though it didn't
    // add an edge. Hmmm.
    let threw = false;
    try {
      event.addEdge(edge);
    } catch (e) {
      threw = true;
    }
    expect(threw).equal(true);
  });

  it('Event.makeFromString() should work', function () {
    const ev0 = aob.Event.makeFromString(evUnsampled);
    const ev1 = aob.Event.makeFromString(evSampled);

    expect(ev0.getSampleFlag()).equal(false);
    expect(ev0.toString()).equal(evUnsampled);
    expect(ev1.getSampleFlag()).equal(true);
    expect(ev1.toString()).equal(evSampled);
  });

  it('should serialize an event string', function () {
    const ev1 = aob.Event.makeFromString(evUnsampled);
    const ev2 = new aob.Event(ev1);
    // task IDs should match
    expect(ev2.toString(2)).equal(ev1.toString(2));
    // op IDs should not match
    expect(ev2.toString(4)).not.equal(ev1.toString(4));
    expect(ev2.toString().substr(0, 2)).equal('2B');
    expect(ev2.toString().slice(-2)).equal('00');
    expect(ev2.toString().length).equal(60);
    expect(ev2.toString()).match(/2B[A-F0-9]{58}/);
  });

  it('should create an event with or without adding an edge', function () {
    const ev1 = new aob.Event(aob.Event.makeRandom(), false);
    const ev2 = new aob.Event(aob.Event.makeRandom(), true);
    expect(ev1).instanceof(aob.Event);
    expect(ev2).instanceof(aob.Event);
  })

  it('should not crash node when getting the prototype of an event', function () {
    const event = aob.Event.makeRandom();
    // eslint-disable-next-line no-unused-vars
    const p = Object.getPrototypeOf(event);

  })
})
