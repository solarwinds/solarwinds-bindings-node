#include "bindings.h"

Napi::FunctionReference Metadata::constructor;

//
// JavaScript callable method to create a new Javascript instance
//
Metadata::Metadata(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Metadata>(info) {
  Napi::Env env = info.Env();

  //Napi::Value Metadata::New(const Napi::CallbackInfo& info) {
  if (!info.IsConstructCall()) {
    Napi::Error::New(env, "Metadata() must be called as a constructor").ThrowAsJavaScriptException();
    return;
  }

  if (info.Length() > 1) {
    Napi::TypeError::New(env, "Invalid signature").ThrowAsJavaScriptException();
  }

  // no argument gets empty metadata
  if (info.Length() == 0) {
    oboe_metadata_init(&metadata);
    return;
  }

  // use a native function to figure out and extract the metadata from one
  // of: Metadata, Event, or String.
  bool ok = Metadata::getMetadata(info[0], &metadata);

  // if it's not any of those it's an error
  if (!ok) {
    Napi::TypeError::New(env, "argument must be Metadata, Event, or a valid X-Trace string");
    return;
  }

}

Metadata::~Metadata() {}

//
// c++ callable constructor
//
Napi::Object Metadata::NewInstance(Napi::Env env, Napi::Value arg) {
    Napi::EscapableHandleScope scope(env);

    Napi::Object o = constructor.New({arg});
    return scope.Escape(napi_value(o)).ToObject();
}

//
// Transform a string back into a metadata instance
//
Napi::Value Metadata::fromString(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string str = info[0].As<Napi::String>();

  oboe_metadata_t omd;
  int status = oboe_metadata_fromstr(&omd, str.c_str(), str.length());
  // status can be zero with a version other than 2, so check that too.
  if (status < 0 || omd.version != 2) {
    return env.Undefined();
  }

  Napi::Value v = Napi::External<oboe_metadata_t>::New(env, &omd);
  return Metadata::NewInstance(env, v);
}

//
// Metadata factory for randomized metadata
//
Napi::Value Metadata::makeRandom(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    oboe_metadata_t omd;

    oboe_metadata_init(&omd);
    oboe_metadata_random(&omd);

    // set or clear the sample flag appropriately if an argument specified.
    if (info.Length() == 1) {
        if (info[0].ToBoolean().Value()) {
            omd.flags |= XTR_FLAGS_SAMPLED;
        } else {
            omd.flags &= ~XTR_FLAGS_SAMPLED;
        }
    }

    Napi::Value v = Napi::External<oboe_metadata_t>::New(env, &omd);

    return Metadata::NewInstance(env, v);
}

//
// Verify that the state of the metadata instance is valid
//
Napi::Value Metadata::isValid(const Napi::CallbackInfo& info) {
    bool status = oboe_metadata_is_valid(&this->metadata);
    return Napi::Boolean::New(info.Env(), status);
}

Napi::Value Metadata::getSampleFlag(const Napi::CallbackInfo& info) {
    return Napi::Boolean::New(info.Env(), this->sampleFlagIsOn());
}

Napi::Value Metadata::setSampleFlagTo(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() != 1) {
        Napi::TypeError::New(env, "requires one argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool previous = this->metadata.flags & XTR_FLAGS_SAMPLED;

    // truthy to set it, falsey to clear it
    if (info[0].ToBoolean().Value()) {
        this->metadata.flags |= XTR_FLAGS_SAMPLED;
    } else {
        this->metadata.flags &= ~XTR_FLAGS_SAMPLED;
    }
    return Napi::Boolean::New(env, previous);
}

//
// JavaScript callable static method to determine if the argument
// (event, metadata, or string) has the sample flag turned on.
//
// returns a boolean indicating the state of the sample flag
//
// if a string argument cannot be converted to metadata it
// returns undefined
//
Napi::Value Metadata::sampleFlagIsSet(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  bool sampleFlag = false;

  // handle each argument type appropriately
  if (info.Length() >= 1) {
    Napi::Object o = info[0].As<Napi::Object>();
    if (Metadata::isMetadata(o)) {
      Metadata* md = Napi::ObjectWrap<Metadata>::Unwrap(o);
      sampleFlag = md->metadata.flags & XTR_FLAGS_SAMPLED;
    } else if (Event::isEvent(o)) {
      Event* e = Napi::ObjectWrap<Event>::Unwrap(o);
      sampleFlag = e->event.metadata.flags & XTR_FLAGS_SAMPLED;
    } else if (info[0].IsString()) {
      std::string str = info[0].As<Napi::String>();

      oboe_metadata_t md;
      int status = oboe_metadata_fromstr(&md, str.c_str(), str.length());
      if (status < 0) {
        return env.Undefined();
      }
      sampleFlag = md.flags & XTR_FLAGS_SAMPLED;
    }
  }

  return Napi::Boolean::New(env, sampleFlag);
}

//
// Serialize a metadata object to a string
//
Napi::Value Metadata::toString(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  char buf[Metadata::fmtBufferSize];

  int rc;
  // no args is non-human-readable form - no delimiters, uppercase
  // if arg == 1 it's the original human readable form.
  // otherwise arg is a bitmask of options.
  if (info.Length() == 0) {
    rc = oboe_metadata_tostr(&this->metadata, buf, sizeof(buf) - 1);
  } else {
    int style = info[0].ToNumber().Int64Value();
    int flags;
    // make style 1 the previous default because ff_header alone is not very useful.
    if (style == 1) {
      flags = Metadata::ff_header | Metadata::ff_task | Metadata::ff_op | Metadata::ff_flags | Metadata::ff_separators | Metadata::ff_lowercase;
    } else {
      // the style are the flags and the separator is a dash.
      flags = style;
    }
    rc = format(&this->metadata, sizeof(buf), buf, flags) ? 0 : -1;
  }

  return Napi::String::New(env, rc == 0 ? buf : "");
}

//
// C++ method to determine if an object is an instance of Metadata
//
bool Metadata::isMetadata(Napi::Object o) {
  return o.IsObject() && o.InstanceOf(constructor.Value());
}

//
// C++ internal method to copy metadata from Object o. Returns
// boolean true if it copied valid oboe_metadata_t, false otherwise.
//
bool Metadata::getMetadata(Napi::Value v, oboe_metadata_t* omd) {
  Napi::Object o = v.ToObject();

  if (Metadata::isMetadata(o)) {
    // it's a metadata instance
    Metadata* md = Napi::ObjectWrap<Metadata>::Unwrap(o);
    *omd = md->metadata;
  } else if (Event::isEvent(o)) {
    // it's an event instance
    Event* e = Napi::ObjectWrap<Event>::Unwrap(o);
    *omd = e->event.metadata;
  } else if (v.IsString()) {
    std::string str = v.As<Napi::String>();

    // it's a string, oboe_metadata_fromstr() can fail and return non-zero.
    oboe_metadata_t md;
    int status = oboe_metadata_fromstr(&md, str.c_str(), str.length());
    if (status < 0) {
      return false;
    }
    *omd = md;
  } else if (v.IsExternal()) {
    oboe_metadata_t* md = v.As<Napi::External<oboe_metadata_t>>().Data();
    *omd = *md;
  } else {
    // it's not something we know how to convert
    return false;
  }

  return true;
}

//
// C++ callable method to get sample flag boolean.
//
bool Metadata::sampleFlagIsOn() {
  return metadata.flags & XTR_FLAGS_SAMPLED;
}

//
// initialize the module and expose the Metadata class.
//
Napi::Object Metadata::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function ctor = DefineClass(env, "Metadata", {
    // Static methods
    StaticMethod("fromString", &Metadata::fromString),
    StaticMethod("makeRandom", &Metadata::makeRandom),
    StaticMethod("sampleFlagIsSet", &Metadata::sampleFlagIsSet),
    // Static values
    StaticValue("fmtHuman", Napi::Number::New(env, Metadata::fmtHuman)),
    StaticValue("fmtLog", Napi::Number::New(env, Metadata::fmtLog)),

    // Instance methods (prototype)
    InstanceMethod("isValid", &Metadata::isValid),
    InstanceMethod("getSampleFlag", &Metadata::getSampleFlag),
    InstanceMethod("setSampleFlagTo", &Metadata::setSampleFlagTo),
    InstanceMethod("toString", &Metadata::toString),
  });

  constructor = Napi::Persistent(ctor);
  constructor.SuppressDestruct();
  exports.Set("Metadata", ctor);

  return exports;
}

//
// function to format an x-trace with components split by sep.
//
bool Metadata::format(oboe_metadata_t* md, size_t len, char* buffer, uint flags) {
    char* b = buffer;
    char base = (flags & Metadata::ff_lowercase ? 'a' : 'A') - 10;
    const char sep = '-';

    auto puthex = [&b, base](uint8_t byte) {
      int digit = (byte >> 4);
      digit += digit <= 9 ? '0' : base;
      *b++ = (char) digit;
      digit = byte & 0xF;
      digit += digit <= 9 ? '0' : base;
      *b++ = (char) digit;

      return b;
    };

    // make sure there is enough room in the buffer.
    // prefix + task + op + flags + 3 colons + null terminator.
    if (2 + md->task_len + md->op_len + 2 + 4 > len) return false;

    const bool separators = flags & Metadata::ff_separators;

    if (flags & Metadata::ff_header) {
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
        if (task_bits == 4) task_bits = 3;
        uint8_t packed = md->version << 4 | ((md->op_len >> 2) - 1) << 3 | task_bits;
        b = puthex(packed);
      }
      // only add a separator if more fields will be output.
      const uint more = Metadata::ff_task | Metadata::ff_op | Metadata::ff_flags | Metadata::ff_sample;
      if (flags & more && separators) {
        *b++ = sep;
      }
    }

    // place to hold pointer to each byte of task and/or op ids.
    uint8_t* mdp;

    // put the task ID
    if (flags & Metadata::ff_task) {
      mdp = (uint8_t *) &md->ids.task_id;
      for(unsigned int i = 0; i < md->task_len; i++) {
          b = puthex(*mdp++);
      }
      if (flags & (Metadata::ff_op | Metadata::ff_flags | Metadata::ff_sample) && separators) {
        *b++ = sep;
      }
    }

    // put the op ID
    if (flags & Metadata::ff_op) {
      mdp = (uint8_t *) &md->ids.op_id;
      for (unsigned int i = 0; i < md->op_len; i++) {
          b = puthex(*mdp++);
      }
      if (flags & (Metadata::ff_flags | Metadata::ff_sample) && separators) {
        *b++ = sep;
      }
    }

    // put the flags byte or sample flag only. flags, if specified, takes precedence over sample.
    if (flags & Metadata::ff_flags) {
      b = puthex(md->flags);
    } else if (flags & Metadata::ff_sample) {
      *b++ = (md->flags & 1) + '0';
    }

    // null terminate the string
    *b = '\0';

    return true;
}
