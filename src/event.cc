#include "bindings.h"
#include "uv.h"
#include <cmath>

#define MAX_SAFE_INTEGER (pow(2, 53) - 1)

Napi::FunctionReference Event::constructor;

Event::~Event() {
  // don't ask oboe to clean up unless the event was successfully created.

  if (initialized) {
    full_active -= 1;
    oboe_event_destroy(&event);
    // send time is only calculated for events that can be sent. it could be calculated in the send function
    // but doing it here keeps the logic together.
    if (send_time) {
      s_sendtime += (send_time - creation_time + 500) / 1000;
    }
  } else {
    small_active -= 1;
  }

  total_destroyed += 1;

  // get event lifetime in microseconds
  uint64_t now = uv_hrtime();
  uint64_t elifetime = (now - creation_time + 500) / 1000;
  // and accumulate it
  lifetime += elifetime;

  // now keep track of memory
  bytes_freed += bytes_allocated;
  total_bytes_alloc -= bytes_allocated;
}

//
// JavaScript constructor
//
// new Event()
// new Event(xtrace, addEdge = true)
//
// @param {Metadata|Event|string} xtrace - X-Trace ID to use for creating event
// @param boolean [addEdge]
//
//
// sizing
// small_active - sizeof (oboe_event_t)
// full_active - sizeof (oboe_event_t) + oboe_event_t->bbuf->bufSize
//
//
Event::Event(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Event>(info) {
    Napi::Env env = info.Env();

    total_created += 1;
    bytes_allocated = sizeof(oboe_event_t);
    total_bytes_alloc += bytes_allocated;
    creation_time = uv_hrtime();
    send_time = 0;

    oboe_metadata_t omd;

    // keep track of whether oboe has initialized the event.
    initialized = false;

    // no argument constructor just makes an empty event. used only by
    // Event::makeRandom() and Event::makeFromString().
    if (info.Length() == 0) {
      small_active += 1;
      return;
    }
    full_active += 1;

    // make sure there is metadata
    if (!info[0].IsObject()) {
      Napi::TypeError::New(env, "invalid signature").ThrowAsJavaScriptException();
      return;
    }

    Napi::Object o = info[0].As<Napi::Object>();

    if (!Event::isEvent(o)) {
      Napi::TypeError::New(env, "argument must be an Event")
          .ThrowAsJavaScriptException();
      return;
    }

    omd = Napi::ObjectWrap<Event>::Unwrap(o)->event.metadata;

    // here there is metadata in omd and that's all the information needed in order
    // to create an event. add an edge if the caller requests.
    bool add_edge = false;
    if (info.Length() >= 2) {
      add_edge = info[1].ToBoolean().Value();
    }

    // supply the metadata for the event. oboe_event_init() will create a new
    // random op ID for the event. (The op ID can be specified using the 3rd
    // argument but there is no benefit to doing so here.)
    int status = oboe_event_init(&this->event, &omd, NULL);

    initialized = status == 0;
    if (!initialized) {
      Napi::Error::New(env, "oboe.event_init: " + std::to_string(status)).ThrowAsJavaScriptException();
      return;
    }
    size_t bb_size = this->event.bbuf.bufSize;
    bytes_allocated += bb_size;
    total_bytes_alloc += bb_size;

    if (add_edge) {
      int edge_status = oboe_event_add_edge(&this->event, &omd);
      if (edge_status != 0) {
        Napi::Error::New(env, "oboe.add_edge: " + std::to_string(edge_status)).ThrowAsJavaScriptException();
        return;
      }
    }
}

//
// C++ callable function to create a non-functional JavaScript Event object. It
// only contains valid metadata; the event structure has not been initialized.
//
Napi::Object Event::NewInstance(const Napi::Env env) {
  Napi::EscapableHandleScope scope(env);

  Napi::Object o = constructor.New({});

  return scope.Escape(Napi::Value(o)).ToObject();
}

//
// Event factory for non-functional event with random metadata.
//
Napi::Value Event::makeRandom(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  // make a JavaScript event and get the underlying C++ class.
  Napi::Object event = Event::NewInstance(env);
  oboe_event_t* oe = &Napi::ObjectWrap<Event>::Unwrap(event)->event;

  // fill it with random data
  oboe_metadata_init(&oe->metadata);
  oboe_metadata_random(&oe->metadata);

  // set or clear the sample flag appropriately if an argument specified.
  if (info.Length() == 1) {
    if (info[0].ToBoolean().Value()) {
      oe->metadata.flags |= XTR_FLAGS_SAMPLED;
    } else {
      oe->metadata.flags &= ~XTR_FLAGS_SAMPLED;
    }
  }

  return event;
}

//
// Event factory for non-functional event with metadata from the supplied
// buffer. This undocumented function requires that
// a valid xtrace id occupies a buffer with a length of 30 bytes
// or that a valid traceparent id occupies a buffer with a length of 26 bytes
//
Napi::Value Event::makeFromBuffer(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!info[0].IsBuffer()) {
    Napi::TypeError::New(env, "argument must be a buffer")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Buffer<uint8_t> b = info[0].As<Napi::Buffer<uint8_t>>();
  if (b.Length() != 26 && b.Length() != 30) {
    Napi::TypeError::New(env, "buffer must from traceparent (26 bytes) or xtrace 30 (bytes")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // make a JavaScript event and get the underlying C++ class.
  Napi::Object event = Event::NewInstance(env);
  oboe_event_t* oe = &Napi::ObjectWrap<Event>::Unwrap(event)->event;

  const uint kHeaderBytes = 1;
  const uint kTaskIdOffset = kHeaderBytes;

  uint kOpIdOffset = kTaskIdOffset + OBOE_MAX_TASK_ID_LEN;
  uint kMaxTaskIdLen = OBOE_MAX_TASK_ID_LEN;
  if(b.Length() == 26) {
    kOpIdOffset = kTaskIdOffset + OBOE_TASK_ID_TRACEPARENT_LEN;
    kMaxTaskIdLen = OBOE_TASK_ID_TRACEPARENT_LEN;
  }

  const uint kFlagsOffset = kOpIdOffset + OBOE_MAX_OP_ID_LEN;

  // copy the bytes from the buffer to the oboe metadata portion
  // of the event.
  oboe_metadata_init(&oe->metadata);
  for (uint i = 0; i < kMaxTaskIdLen; i++) {
    oe->metadata.ids.task_id[i] = b[kTaskIdOffset + i];
  }
  for (uint i = 0; i < OBOE_MAX_OP_ID_LEN; i++) {
    oe->metadata.ids.op_id[i] = b[kOpIdOffset + i];
  }
  oe->metadata.flags = b[kFlagsOffset];

  return event;
}

//
// C++ callable function to create an event with specific metadata
//
Napi::Object Event::makeFromOboeMetadata(const Napi::Env env, oboe_metadata_t &omd) {
  // make a JavaScript event and get the underlying C++ class.
  Napi::Object event = Event::NewInstance(env);
  oboe_event_t* oe = &Napi::ObjectWrap<Event>::Unwrap(event)->event;
  oe->metadata = omd;
  return event;
}

//
// JavaScript callable method to get the sample flag from
// the oboe event metadata.
//
// returns boolean
//
Napi::Value Event::getSampleFlag(const Napi::CallbackInfo& info) {
  const bool sampleFlag = this->event.metadata.flags & XTR_FLAGS_SAMPLED;
  return Napi::Boolean::New(info.Env(), sampleFlag);
}

//
// JavaScript callable method to add an edge to the event.
//
// event.addEdge(edge)
//
// @param {Event | string} X-Trace ID to edge back to
//
Napi::Value Event::addEdge(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments. If init status is not 0 then this is a
    // non-functional, metadata-only event.
    if (info.Length() != 1 || !initialized) {
        Napi::TypeError::New(env, "invalid signature").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    size_t bb_size = this->event.bbuf.bufSize;

    int status;
    Napi::Object o = info[0].ToObject();
    // is it an Event?
    if (Event::isEvent(o)) {
      Event* e = Napi::ObjectWrap<Event>::Unwrap(o);
      status = oboe_event_add_edge(&this->event, &e->event.metadata);
    } else if (info[0].IsString()) {
        std::string str = info[0].As<Napi::String>();
        status = oboe_event_add_edge_fromstr(&this->event, str.c_str(), str.length());
    } else {
        Napi::TypeError::New(env, "invalid edge").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // adjust the bytes allocated in case the buffer size changed.
    if ((unsigned)this->event.bbuf.bufSize != bb_size) {
      size_t delta = this->event.bbuf.bufSize - bb_size;
      bytes_allocated += delta;
      total_bytes_alloc += delta;
    }

    if (status < 0) {
        Napi::Error::New(env, "Failed to add edge").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    return Napi::Boolean::New(env, true);
}

//
// JavaScript method to add info to the event.
//
// event.addInfo(key, value)
//
// @param {string} key
// @param {string | number | boolean} value
//
Napi::Value Event::addInfo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments
    if (info.Length() != 2 || !info[0].IsString() || !initialized) {
        Napi::TypeError::New(env, "Invalid signature").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    oboe_event_t* event = &this->event;

    int status;

    size_t bb_size = this->event.bbuf.bufSize;

    // Get key string
    std::string key = info[0].As<Napi::String>();

    if (info[1].IsBoolean()) {
      bool v = info[1].As<Napi::Boolean>().Value();
      status = oboe_event_add_info_bool(event, key.c_str(), v);
    } else if (info[1].IsNumber()) {
      const double v = info[1].As<Napi::Number>();
      double v_int;
      // if it has a fractional part or is outside the range of integer values
      // it's a double.
      double v_frac = std::modf(v, &v_int);
      if (v_frac != 0 || v > MAX_SAFE_INTEGER || v < -MAX_SAFE_INTEGER) {
        status = oboe_event_add_info_double(event, key.c_str(), v);
      } else {
        status = oboe_event_add_info_int64(event, key.c_str(), v);
      }
    } else if (info[1].IsString()) {
      std::string str = info[1].As<Napi::String>();
      // binary is not really binary, it's utf8. but we don't want any embedded nulls so
      // just use oboe_event_add_info.
      status = oboe_event_add_info(event, key.c_str(), str.c_str());
    } else {
      Napi::TypeError::New(env, "Value must be a boolean, string or number")
          .ThrowAsJavaScriptException();
      return env.Undefined();
    }

    // adjust the bytes allocated in case the buffer size changed.
    if ((unsigned)this->event.bbuf.bufSize != bb_size) {
      size_t delta = this->event.bbuf.bufSize - bb_size;
      bytes_allocated += delta;
      total_bytes_alloc += delta;
    }

    if (status < 0) {
      Napi::Error::New(env, "Failed to add info").ThrowAsJavaScriptException();
    }

    return Napi::Boolean::New(env, status == 0);
}

Napi::Value Event::getBytesAllocated(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), bytes_allocated);
}

Napi::Value Event::getEventStats(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  int flags = 0;

  if (info.Length() == 1 && info[0].IsNumber()) {
    flags = info[0].ToNumber();
  }

  Napi::Object o = Napi::Object::New(env);
  // this metric rises but cannot be reset
  o.Set("totalCreated", Napi::Number::New(env, total_created));

  // these metrics rise and fall
  o.Set("totalDestroyed", Napi::Number::New(env, total_destroyed));
  o.Set("smallActive", Napi::Number::New(env, small_active));
  o.Set("fullActive", Napi::Number::New(env, full_active));
  o.Set("totalBytesAllocated", Napi::Number::New(env, total_bytes_alloc));

  // calculate some metrics on the fly
  size_t events_active = total_created - total_destroyed;
  o.Set("totalActive", Napi::Number::New(env, events_active));

  // these are only calculated when an event is destroyed so don't calculate
  // them unless some events have been destroyed.
  size_t delta_destroyed = total_destroyed - ptotal_destroyed;
  double average_lifetime = 0;
  if (delta_destroyed != 0) {
    average_lifetime = (lifetime - plifetime) / delta_destroyed;
    ptotal_destroyed = total_destroyed;
  }
  o.Set("averageLifetime", Napi::Number::New(env, average_lifetime));

  double average_sendtime = 0;
  size_t delta_sent = sent_count - s_psent_count;
  if (delta_sent != 0) {
    average_sendtime = (s_sendtime - s_psendtime) / delta_sent;
  }
  o.Set("averageSendtime", Napi::Number::New(env, average_sendtime));

  // these metrics only rise if not reset
  o.Set("bytesUsed", Napi::Number::New(env, actual_bytes_used));
  o.Set("sentCount", Napi::Number::New(env, sent_count));
  o.Set("lifetime", Napi::Number::New(env, lifetime));
  o.Set("sendtime", Napi::Number::New(env, s_sendtime));
  o.Set("bytesFreed", Napi::Number::New(env, bytes_freed));

  // reset these if requested
  if (flags & 0x01) {
    actual_bytes_used = 0;
    sent_count = 0;
    lifetime = 0;
    s_sendtime = 0;
    bytes_freed = 0;
  }

  // and remember the previous values used for averages.
  plifetime = lifetime;
  s_psendtime = s_sendtime;
  s_psent_count = sent_count;

  return o;
}

//
// C++ callable method to determine if object is a JavaScript Event
// instance.
//
inline bool Event::isEvent(Napi::Object o) {
  return o.InstanceOf(constructor.Value());
}

size_t Event::total_created;
size_t Event::total_destroyed;
size_t Event::ptotal_destroyed;
size_t Event::bytes_freed;
size_t Event::total_bytes_alloc;
size_t Event::small_active;
size_t Event::full_active;
size_t Event::actual_bytes_used;
size_t Event::sent_count;
size_t Event::s_psent_count;
size_t Event::lifetime;
size_t Event::plifetime;
size_t Event::s_sendtime;
size_t Event::s_psendtime;

//
// initialize the module and expose the Event class.
//
Napi::Object Event::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);
  total_created = 0;
  total_destroyed = 0;
  ptotal_destroyed = 0;
  bytes_freed = 0;
  total_bytes_alloc = 0;
  small_active = 0;
  full_active = 0;
  actual_bytes_used = 0;
  sent_count = 0;
  s_psent_count = 0;
  lifetime = 0;
  plifetime = 0;
  s_sendtime = 0;
  s_psendtime = 0;

  Napi::Function ctor = DefineClass(
      env, "Event", {
        InstanceMethod("addInfo", &Event::addInfo),
        InstanceMethod("addEdge", &Event::addEdge),
        InstanceMethod("toString", &Event::toString),
        InstanceMethod("getSampleFlag", &Event::getSampleFlag),
        InstanceMethod("sendReport", &Event::sendReport),
        InstanceMethod("sendStatus", &Event::sendStatus),
        InstanceMethod("getBytesAllocated", &Event::getBytesAllocated),

        StaticValue("fmtHuman", Napi::Number::New(env, Event::fmtHuman)),
        StaticValue("fmtLog", Napi::Number::New(env, Event::fmtLog)),

        StaticMethod("makeRandom", &Event::makeRandom),
        StaticMethod("makeFromBuffer", &Event::makeFromBuffer),
        StaticMethod("getEventStats", &Event::getEventStats),
      }
    );

  constructor = Napi::Persistent(ctor);
  constructor.SuppressDestruct();

  exports.Set("Event", ctor);

  return exports;
}
