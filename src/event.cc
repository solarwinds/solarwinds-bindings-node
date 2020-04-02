#include "bindings.h"
#include <cmath>

#define MAX_SAFE_INTEGER (pow(2, 53) - 1)

Napi::FunctionReference Event::constructor;

Event::~Event() {
  // don't ask oboe to clean up unless the event was successfully created.
  if (init_status == 0) {
    oboe_event_destroy(&event);
  }
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
Event::Event(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Event>(info) {
    Napi::Env env = info.Env();

    oboe_metadata_t omd;

    // indicate that the construction failed so the destructor won't ask oboe
    // to destroy the event.
    init_status = -1;

    // no argument constructor just makes an empty event. used only by
    // Event::makeRandom() and Event::makeFromString().
    if (info.Length() == 0) {
      return;
    }

    // make sure there is metadata
    if (!info[0].IsObject()) {
      Napi::TypeError::New(env, "invalid signature").ThrowAsJavaScriptException();
      return;
    }

    Napi::Object o = info[0].As<Napi::Object>();

    if (!o.InstanceOf(constructor.Value())) {
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
    init_status = oboe_event_init(&this->event, &omd, NULL);
    if (init_status != 0) {
      Napi::Error::New(env, "oboe.event_init: " + std::to_string(init_status)).ThrowAsJavaScriptException();
      return;
    }

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
Napi::Object Event::NewInstance(Napi::Env env) {
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
// buffer. This undocumented function requires that a valid xtrace id
// occupies a buffer with a length of 30 bytes.
//
Napi::Value Event::makeFromBuffer(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!info[0].IsBuffer()) {
    Napi::TypeError::New(env, "argument must be a buffer")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }
  Napi::Buffer<uint8_t> b = info[0].As<Napi::Buffer<uint8_t>>();
  if (b.Length() != 30) {
    Napi::TypeError::New(env, "buffer must be length 30")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // make a JavaScript event and get the underlying C++ class.
  Napi::Object event = Event::NewInstance(env);
  oboe_event_t* oe = &Napi::ObjectWrap<Event>::Unwrap(event)->event;

  const uint kHeaderBytes = 1;
  const uint kTaskIdOffset = kHeaderBytes;
  const uint kOpIdOffset = kTaskIdOffset + OBOE_MAX_TASK_ID_LEN;
  const uint kFlagsOffset = kOpIdOffset + OBOE_MAX_OP_ID_LEN;

  // copy the bytes from the buffer to the oboe metadata portion
  // of the event.
  oboe_metadata_init(&oe->metadata);
  for (uint i = 0; i < OBOE_MAX_TASK_ID_LEN; i++) {
    oe->metadata.ids.task_id[i] = b[kTaskIdOffset + i];
  }
  for (uint i = 0; i < OBOE_MAX_OP_ID_LEN; i++) {
    oe->metadata.ids.op_id[i] = b[kOpIdOffset + i];
  }
  oe->metadata.flags = b[kFlagsOffset];

  return event;
}

//
// C++ callable function to create a JavaScript Event object.
//
// This signature includes an optional boolean for whether an edge
// should be set or not. It defaults to true.
//
//Napi::Object Event::NewInstance(Napi::Env env,
//                                oboe_metadata_t* omd,
//                                bool edge) {
//  Napi::EscapableHandleScope scope(env);
//
//  Napi::Value v0 = Napi::External<oboe_metadata_t>::New(env, omd);
//  Napi::Value v1 = Napi::Boolean::New(env, edge);
//
//  Napi::Object o = constructor.New({v0, v1});
//
//  return scope.Escape(napi_value(o)).ToObject();
//}

//
// JavaScript callable method to format the event as a string.
//
// The string representation of an event is the string representation of
// the metadata part of the event.
//
//Napi::Value Event::toString(const Napi::CallbackInfo& info) {
//    oboe_metadata_t* md = &this->event.metadata;
//    char buf[Metadata::fmtBufferSize];
//
//    int rc;
//
//    if (info.Length() == 1 && info[0].IsNumber()) {
//      int fmt = info[0].As<Napi::Number>().Int64Value();
//      // default 1 to a human readable form for historical reasons.
//      if (fmt == 1) {
//        fmt = Metadata::fmtHuman;
//      }
//      rc = Metadata::format(md, sizeof(buf), buf, fmt) ? 0 : -1;
//    } else {
//      rc = oboe_metadata_tostr(md, buf, sizeof(buf) - 1);
//    }
//
//    return Napi::String::New(info.Env(), rc == 0 ? buf : "");
//}

//
// JavaScript callable method to set the event's sample flag to the boolean
// sense of the argument.
//
// returns the previous value of the flag.
//
//Napi::Value Event::setSampleFlagTo(const Napi::CallbackInfo& info) {
//    Napi::Env env = info.Env();
//
//    if (info.Length() != 1) {
//        Napi::Error::New(env, "setSampleFlagTo requires one argument").//ThrowAsJavaScriptException();
//        return env.Null();
//    }
//
//    bool previous = this->event.metadata.flags & XTR_FLAGS_SAMPLED;
//
//    // truthy to set it, falsey to clear it
//    if (info[0].ToBoolean().Value()) {
//        this->event.metadata.flags |= XTR_FLAGS_SAMPLED;
//    } else {
//        this->event.metadata.flags &= ~XTR_FLAGS_SAMPLED;
//    }
//    return Napi::Boolean::New(env, previous);
//}

//
// JavaScript callable method to get the sample flag from
// the event metadata.
//
// returns boolean
//
Napi::Value Event::getSampleFlag(const Napi::CallbackInfo& info) {
  const bool sampleFlag = this->event.metadata.flags & XTR_FLAGS_SAMPLED;
  return Napi::Boolean::New(info.Env(), sampleFlag);
}

//
// JavaScript callable method to get a new metadata object with
// the same metadata as this event.
//
// Event.getMetadata()
//
// Return a Metadata object with the metadata from this event.
//
//Napi::Value Event::getMetadata(const Napi::CallbackInfo& info) {
//    Napi::Env env = info.Env();
//
//    oboe_metadata_t* omd = &this->event.metadata;
//
//    Napi::Value v = Napi::External<oboe_metadata_t>::New(env, omd);
//    return Metadata::NewInstance(env, v);
//}

//
// JavaScript callable method to add an edge to the event.
//
// event.addEdge(edge)
//
// @param {Event | string} X-Trace ID to edge back to
//
Napi::Value Event::addEdge(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // Validate arguments
    if (info.Length() != 1) {
        Napi::TypeError::New(env, "invalid signature").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    int status;
    Napi::Object o = info[0].ToObject();
    // is it an Event?
    if (o.InstanceOf(constructor.Value())) {
      Event* e = Napi::ObjectWrap<Event>::Unwrap(o);
      status = oboe_event_add_edge(&this->event, &e->event.metadata);
      //    } else if (Metadata::isMetadata(o)) {
      //        Metadata* md = Napi::ObjectWrap<Metadata>::Unwrap(o);
      //        status = oboe_event_add_edge(&this->event, &md->metadata);
    } else if (info[0].IsString()) {
        std::string str = info[0].As<Napi::String>();
        status = oboe_event_add_edge_fromstr(&this->event, str.c_str(), str.length());
    } else {
        Napi::TypeError::New(env, "invalid metadata").ThrowAsJavaScriptException();
        return env.Undefined();
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
    if (info.Length() != 2 || !info[0].IsString()) {
        Napi::TypeError::New(env, "Invalid signature").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    oboe_event_t* event = &this->event;

    int status;

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
      // binary is not really binary, it's utf8.
      status = oboe_event_add_info_binary(event, key.c_str(), str.c_str(), str.length());
    } else {
      Napi::TypeError::New(env, "Value must be a boolean, string or number")
          .ThrowAsJavaScriptException();
      return env.Undefined();
    }

    if (status < 0) {
        Napi::Error::New(env, "Failed to add info").ThrowAsJavaScriptException();
    }

    return Napi::Boolean::New(env, status == 0);
}

//
// Convert an event's metadata to a string representation.
//
Napi::Value Event::toString(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  char buf[Event::fmtBufferSize];

  int rc;
  // no args is non-human-readable form - no delimiters, uppercase
  // if arg == 1 it's the original human readable form.
  // otherwise arg is a bitmask of options.
  if (info.Length() == 0) {
    rc = oboe_metadata_tostr(&this->event.metadata, buf, sizeof(buf) - 1);
  } else {
    int style = info[0].ToNumber().Int64Value();
    int flags;
    // make style 1 the previous default because ff_header alone is not very
    // useful.
    if (style == 1) {
      flags = Event::ff_header | Event::ff_task | Event::ff_op |
              Event::ff_flags | Event::ff_separators |
              Event::ff_lowercase;
    } else {
      // the style are the flags and the separator is a dash.
      flags = style;
    }
    rc = format(&this->event.metadata, sizeof(buf), buf, flags) ? 0 : -1;
  }

  return Napi::String::New(env, rc == 0 ? buf : "");
}

//
// core formatting code
//
//
// function to format an x-trace with components split by sep.
//
int Event::format(oboe_metadata_t* md,
                      size_t len,
                      char* buffer,
                      uint flags) {
  char* b = buffer;
  char base = (flags & Event::ff_lowercase ? 'a' : 'A') - 10;
  const char sep = '-';

  auto puthex = [&b, base](uint8_t byte) {
    int digit = (byte >> 4);
    digit += digit <= 9 ? '0' : base;
    *b++ = (char)digit;
    digit = byte & 0xF;
    digit += digit <= 9 ? '0' : base;
    *b++ = (char)digit;

    return b;
  };

  // make sure there is enough room in the buffer.
  // prefix + task + op + flags + 3 colons + null terminator.
  if (2 + md->task_len + md->op_len + 2 + 4 > len)
    return false;

  const bool separators = flags & Event::ff_separators;

  if (flags & Event::ff_header) {
    if (md->version == 2) {
      // the encoding of the task and op lengths is kind of silly so
      // skip it if version two. it seems like using the whole byte for
      // a major/minor version then tying the length to the version makes
      // more sense. what's the point of the version if it's not used to
      // make decisions in the code?
      b = puthex(0x2b);
    } else {
      // fix up the first byte using arcane length encoding rules.
      uint task_bits = (md->task_len >> 2) - 1;
      if (task_bits == 4)
        task_bits = 3;
      uint8_t packed =
          md->version << 4 | ((md->op_len >> 2) - 1) << 3 | task_bits;
      b = puthex(packed);
    }
    // only add a separator if more fields will be output.
    const uint more = Event::ff_task | Event::ff_op | Event::ff_flags |
                      Event::ff_sample;
    if (flags & more && separators) {
      *b++ = sep;
    }
  }

  // place to hold pointer to each byte of task and/or op ids.
  uint8_t* mdp;

  // put the task ID
  if (flags & Event::ff_task) {
    mdp = (uint8_t*)&md->ids.task_id;
    for (unsigned int i = 0; i < md->task_len; i++) {
      b = puthex(*mdp++);
    }
    if (flags & (Event::ff_op | Event::ff_flags | Event::ff_sample) &&
        separators) {
      *b++ = sep;
    }
  }

  // put the op ID
  if (flags & Event::ff_op) {
    mdp = (uint8_t*)&md->ids.op_id;
    for (unsigned int i = 0; i < md->op_len; i++) {
      b = puthex(*mdp++);
    }
    if (flags & (Event::ff_flags | Event::ff_sample) && separators) {
      *b++ = sep;
    }
  }

  // put the flags byte or sample flag only. flags, if specified, takes
  // precedence over sample.
  if (flags & Event::ff_flags) {
    b = puthex(md->flags);
  } else if (flags & Event::ff_sample) {
    *b++ = (md->flags & 1) + '0';
  }

  // null terminate the string
  *b++ = '\0';

  // return bytes written
  return b - buffer;
}

//
// Send an event to the reporter
//
Napi::Value Event::sendReport(const Napi::CallbackInfo& info) {
  int status = send_event_x(OBOE_SEND_EVENT);
  return Napi::Number::New(info.Env(), status);
}

//
// send status. only used for init message.
//
Napi::Value Event::sendStatus(const Napi::CallbackInfo& info) {
  int status = send_event_x(OBOE_SEND_STATUS);
  return Napi::Number::New(info.Env(), status);
}

//
// Common code for sendReport and sendStatus.
//
int Event::send_event_x(int channel) {

  // fake up metadata so oboe can check it. change the op_id so it doesn't
  // match the event's in oboe's check.
  oboe_metadata_t omd = this->event.metadata;
  omd.ids.op_id[0] += 1;

  int status;

  status = oboe_event_add_timestamp(&this->event);
  if (status < 0) {
    return -1000;
  }
  status = oboe_event_add_hostname(&this->event);
  if (status < 0) {
    return -1001;
  }

  // finalize the bson buffer and send the message
  this->event.bb_str = oboe_bson_buffer_finish(&this->event.bbuf);
  if (this->event.bb_str) {
    return -1002;
  }
  size_t len = &this->event.bbuf.cur - &this->event.bbuf.buf;

  status = oboe_raw_send(channel, this->event.bb_str, len);

  return status;
}

//
// C++ callable method to determine if object is a JavaScript Event
// instance.
//
bool Event::isEvent(Napi::Object o) {
  return o.IsObject() && o.InstanceOf(constructor.Value());
}

//
// initialize the module and expose the Event class.
//
Napi::Object Event::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function ctor = DefineClass(env, "Event", {
    InstanceMethod("addInfo", &Event::addInfo),
    InstanceMethod("addEdge", &Event::addEdge),
    //InstanceMethod("getMetadata", &Event::getMetadata),
    InstanceMethod("toString", &Event::toString),
    InstanceMethod("getSampleFlag", &Event::getSampleFlag),
    //InstanceMethod("setSampleFlagTo", &Event::setSampleFlagTo),

    StaticMethod("makeRandom", &Event::makeRandom),
    StaticMethod("makeFromBuffer", &Event::makeFromBuffer)
  });

  constructor = Napi::Persistent(ctor);
  constructor.SuppressDestruct();

  exports.Set("Event", ctor);

  return exports;
}
